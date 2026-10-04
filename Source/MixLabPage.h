// v0.41 MIX LAB page (included by PluginEditor.cpp - uses its HotButton, RackKnob, pageBackdrop, TC)
// SHAPE EQ / PUNCH COMP / TIME MACHINE on the left, the COACH always on the right.

// ---------------- the analyzer: one FFT of what goes in and what comes out, shared by the EQ and the COACH ----------------
struct MixAnalyzer
{
    static constexpr int order = 12, size = 1 << order;
    dsp::FFT fft { order };
    std::array<float, size> win {};
    std::vector<float> work = std::vector<float> ((size_t) size * 2, 0.0f);
    std::vector<float> preDb = std::vector<float> ((size_t) size / 2 + 1, -120.0f), postDb = preDb, longPow = std::vector<float> ((size_t) size / 2 + 1, 0.0f);
    std::vector<float> persist = std::vector<float> ((size_t) size / 2 + 1, 0.0f), frameDb = persist;   // how often each bin sticks out (resonances)
    float sr = 44100;
    MixAnalyzer() { for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / (float) (size - 1)); }
    void run (const std::array<float, kk::MixLabState::ringSize>& ring, int w, std::vector<float>& db, std::vector<float>* lp)
    {
        constexpr int R = kk::MixLabState::ringSize;
        for (int i = 0; i < size; ++i) work[(size_t) i] = ring[(size_t) ((w - size + i + R) & (R - 1))] * win[(size_t) i];
        std::fill (work.begin() + size, work.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (work.data(), true);
        const float norm = 4.0f / (float) size;
        for (int b = 0; b <= size / 2; ++b)
        {
            const float m = work[(size_t) b] * norm;
            const float d = 20.0f * std::log10 (m + 1e-9f);
            auto& v = db[(size_t) b];
            v = d > v ? 0.6f * v + 0.4f * d : 0.88f * v + 0.12f * d;   // quick up, slow down
            if (lp != nullptr) { (*lp)[(size_t) b] = 0.985f * (*lp)[(size_t) b] + 0.015f * m * m; frameDb[(size_t) b] = d; }
        }
        if (lp != nullptr && db[(size_t) size / 8] > -110.0f)
            for (int b = 8; b < size / 2 - 8; ++b)
            {
                const int span = jmax (4, b / 6);
                float s = 0; int c = 0;
                for (int k = b - span; k <= b + span; k += 2) if (k > 0 && k < size / 2 && std::abs (k - b) > 1) { s += frameDb[(size_t) k]; ++c; }
                const bool out = frameDb[(size_t) b] - s / (float) jmax (1, c) > 8.0f;
                persist[(size_t) b] = 0.97f * persist[(size_t) b] + (out ? 0.03f : 0.0f);
            }
    }
    void update (kk::MixLabState& st, float rate)
    {
        sr = rate > 0 ? rate : 44100.0f;
        const int w = st.ringW.load (std::memory_order_acquire);
        run (st.preRing, w, preDb, nullptr);
        run (st.postRing, w, postDb, &longPow);
    }
    // display value at hz (dB, with a +4.5 dB/oct tilt so music looks level)
    float at (const std::vector<float>& db, float hz) const
    {
        const float bin = hz / (sr * 0.5f) * (float) (size / 2);
        const int b0 = jlimit (1, size / 2 - 1, (int) bin);
        const int span = jmax (0, (int) (bin * 0.03f));
        float mx = -120.0f; for (int b = jmax (1, b0 - span); b <= jmin (size / 2, b0 + span + 1); ++b) mx = jmax (mx, db[(size_t) b]);
        return mx + 4.5f * std::log2 (hz / 1000.0f);
    }
};

static float mixX (float hz, Rectangle<float> r) { return r.getX() + r.getWidth() * std::log (hz / 20.0f) / std::log (1000.0f); }
static float mixHz (float x, Rectangle<float> r) { return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static Colour bandColour (int b)
{
    static const uint32 c[] { 0xffff4d6d, 0xffff8a3d, 0xffffd23f, 0xff7cf56a, 0xff22d3ee, 0xff4d7dff, 0xffa78bfa, 0xffff4fd8 };
    return Colour (c[jlimit (0, 7, b)]);
}

// ---------------- SHAPE EQ display: analyzer + curve + nodes you drag ----------------
class EqDisplay : public Component, public SettableTooltipClient
{
public:
    EqDisplay (KeysKillaProcessor& p, MixAnalyzer& a) : proc (p), an (a)
    {
        setTooltip ("Drag a dot = frequency + gain.  Wheel = width (Q).  Double-click empty space = a new band, double-click a dot = off.  Right-click a dot = type, slope, DYNAMIC.");
    }
    std::function<void (int)> onSelect;
    int sel = 3;
    std::vector<kk::coach::EqSnap> variants;   // EVOLVE thumbnails (drawn by the page)
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto r = getLocalBounds().toFloat();
        kk::modern::well (g, r, 12.0f);
        g.setColour (Colour (0xff0b0d12).withAlpha (t.night ? 0.55f : 0.82f)); g.fillRoundedRectangle (r.reduced (2), 11.0f);
        const auto a = area();
        // grid
        g.setFont (kk::modern::font (10.5f, true, 0.05f));
        for (float hz : { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
        {
            const float x = mixX (hz, a);
            g.setColour (Colours::white.withAlpha (hz == 100.0f || hz == 1000.0f || hz == 10000.0f ? 0.13f : 0.06f)); g.fillRect (x, a.getY(), 1.0f, a.getHeight());
            g.setColour (Colours::white.withAlpha (0.45f)); g.drawText (kk::coach::hzText (hz).replace (" ", ""), (int) x - 26, (int) a.getBottom() + 2, 52, 14, Justification::centred);
        }
        for (int db = -18; db <= 18; db += 6)
        {
            const float y = yOf ((float) db, a);
            g.setColour (Colours::white.withAlpha (db == 0 ? 0.16f : 0.05f)); g.fillRect (a.getX(), y, a.getWidth(), 1.0f);
            g.setColour (Colours::white.withAlpha (0.4f)); g.drawText ((db > 0 ? "+" : "") + String (db), (int) a.getRight() + 4, (int) y - 7, 30, 14, Justification::centredLeft);
        }
        // analyzer: in (dim) and out (glowing gradient)
        auto spec = [&] (const std::vector<float>& db, bool fill) -> Path
        {
            Path p; bool first = true;
            for (float x = a.getX(); x <= a.getRight(); x += 2.0f)
            {
                const float v = an.at (db, mixHz (x, a));
                const float y = jlimit (a.getY(), a.getBottom(), a.getY() + a.getHeight() * (-(v + 6.0f) / 84.0f));
                if (first) { p.startNewSubPath (x, fill ? a.getBottom() : y); if (fill) p.lineTo (x, y); first = false; } else p.lineTo (x, y);
            }
            if (fill) { p.lineTo (a.getRight(), a.getBottom()); p.closeSubPath(); }
            return p;
        };
        g.setColour (Colours::white.withAlpha (0.18f)); g.strokePath (spec (an.preDb, false), PathStrokeType (1.0f));
        auto post = spec (an.postDb, true);
        g.setGradientFill (ColourGradient (Colour (0xff22d3ee).withAlpha (0.36f), a.getX(), 0, Colour (0xffa78bfa).withAlpha (0.36f), a.getRight(), 0, false));
        g.fillPath (post);
        g.setGradientFill (ColourGradient (Colour (0xff22d3ee).withAlpha (0.85f), a.getX(), 0, Colour (0xffff4fd8).withAlpha (0.85f), a.getRight(), 0, false));
        g.strokePath (post, PathStrokeType (1.1f));
        // each band's own shape (soft, coloured), then the sum
        const auto s = kk::coach::snap (proc.mixLab);
        const float sr = an.sr;
        for (int b = 0; b < kk::MixLabState::numBands; ++b)
        {
            if (! s.b[(size_t) b].on) continue;
            const auto& bb = s.b[(size_t) b];
            const auto c = kk::eqCoefs (bb.type, bb.freq, bb.gain, bb.q, sr);
            Path p; bool first = true;
            for (float x = a.getX(); x <= a.getRight(); x += 3.0f)
            {
                const float y = yOf (kk::eqMagDb (c, mixHz (x, a), sr) * ((bb.type == kk::eqLowCut || bb.type == kk::eqHighCut) ? (float) bb.slope : 1.0f), a);
                if (first) { p.startNewSubPath (x, yOf (0, a)); p.lineTo (x, y); first = false; } else p.lineTo (x, y);
            }
            p.lineTo (a.getRight(), yOf (0, a)); p.closeSubPath();
            g.setColour (bandColour (b).withAlpha (b == sel ? 0.28f : 0.12f)); g.fillPath (p);
            // DYNAMIC: how much it cuts right now
            const float dyn = proc.mixLab.mDyn[(size_t) b].load();
            if (bb.dyn > 0.001f && dyn < -0.2f)
            {
                const auto cd = kk::eqCoefs (bb.type, bb.freq, bb.gain + dyn, bb.q, sr);
                Path pd; bool f2 = true;
                for (float x = mixX (bb.freq / 4, a); x <= mixX (bb.freq * 4, a); x += 2.0f)
                {
                    const float y = yOf (kk::eqMagDb (cd, mixHz (x, a), sr), a);
                    if (f2) { pd.startNewSubPath (x, y); f2 = false; } else pd.lineTo (x, y);
                }
                g.setColour (bandColour (b).brighter (0.4f)); g.strokePath (pd, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
            }
        }
        Path curve; bool first = true;
        for (float x = a.getX(); x <= a.getRight(); x += 2.0f)
        {
            const float y = yOf (kk::coach::curveDb (s, mixHz (x, a), sr), a);
            if (first) { curve.startNewSubPath (x, y); first = false; } else curve.lineTo (x, y);
        }
        g.setColour (Colours::white.withAlpha (0.25f)); g.strokePath (curve, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white); g.strokePath (curve, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        // nodes
        for (int b = 0; b < kk::MixLabState::numBands; ++b)
        {
            const auto pt = node (b, a);
            const bool on = s.b[(size_t) b].on;
            const float rr = b == sel ? 9.0f : 7.0f;
            if (on) { g.setColour (bandColour (b).withAlpha (0.3f)); g.fillEllipse (pt.x - rr - 4, pt.y - rr - 4, (rr + 4) * 2, (rr + 4) * 2); }
            g.setColour (on ? bandColour (b) : Colours::white.withAlpha (0.18f)); g.fillEllipse (pt.x - rr, pt.y - rr, rr * 2, rr * 2);
            g.setColour (on ? Colours::black.withAlpha (0.8f) : Colours::white.withAlpha (0.5f)); g.setFont (kk::modern::font (11.0f, true, 0.0f));
            g.drawText (String (b + 1), Rectangle<float> (pt.x - rr, pt.y - rr, rr * 2, rr * 2), Justification::centred);
            if (on && s.b[(size_t) b].dyn > 0.001f) { g.setColour (bandColour (b)); g.drawEllipse (pt.x - rr - 3, pt.y - rr - 3, (rr + 3) * 2, (rr + 3) * 2, 1.2f); }
        }
        // hover read-out: frequency, note, gain
        if (hover.x > a.getX() && hover.x < a.getRight() && hover.y > a.getY() && hover.y < a.getBottom())
        {
            const float hz = mixHz (hover.x, a);
            const String txt = kk::coach::hzText (hz) + "  " + kk::coach::noteOf (hz) + "   " + String (dbOf (hover.y, a), 1) + " dB";
            g.setColour (Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (a.getRight() - 200, a.getY() + 6, 194, 22, 6);
            g.setColour (Colours::white); g.setFont (kk::modern::font (12.0f, true, 0.04f));
            g.drawText (txt, Rectangle<float> (a.getRight() - 200, a.getY() + 6, 194, 22), Justification::centred);
        }
    }
    void mouseMove (const MouseEvent& e) override { hover = e.position; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = { -1, -1 }; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        drag = nodeAt (e.position);
        if (drag >= 0) { sel = drag; if (onSelect) onSelect (sel); }
        if (drag >= 0 && e.mods.isPopupMenu()) { bandMenu (drag); drag = -1; }
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto a = area();
        auto& b = proc.mixLab.band[(size_t) drag];
        b.on = true;
        b.freq = jlimit (20.0f, 20000.0f, mixHz (e.position.x, a));
        if (b.type.load() != kk::eqLowCut && b.type.load() != kk::eqHighCut && b.type.load() != kk::eqNotch) b.gain = jlimit (-18.0f, 18.0f, dbOf (e.position.y, a));
        if (onSelect) onSelect (drag);
        repaint();
    }
    void mouseUp (const MouseEvent&) override { drag = -1; }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        const int n = nodeAt (e.position);
        if (n >= 0) { proc.mixLab.band[(size_t) n].on = ! proc.mixLab.band[(size_t) n].on.load(); if (onSelect) onSelect (n); repaint(); return; }
        const auto a = area();
        const float hz = mixHz (e.position.x, a);
        int best = -1; float bd = 1e9f;
        for (int b = 1; b < kk::MixLabState::numBands - 1; ++b)
            if (! proc.mixLab.band[(size_t) b].on.load()) { const float d = std::abs (std::log2 (proc.mixLab.band[(size_t) b].freq.load() / hz)); if (d < bd) { bd = d; best = b; } }
        if (best < 0) return;
        auto& b = proc.mixLab.band[(size_t) best];
        b.on = true; b.type = kk::eqBell; b.freq = hz; b.gain = jlimit (-18.0f, 18.0f, dbOf (e.position.y, a)); b.q = 1.0f; b.dyn = 0;
        proc.mixLab.eqOn = true;
        sel = best; if (onSelect) onSelect (best);
        repaint();
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const int n = nodeAt (e.position) >= 0 ? nodeAt (e.position) : sel;
        auto& b = proc.mixLab.band[(size_t) n];
        b.q = jlimit (0.1f, 24.0f, b.q.load() * std::pow (2.0f, w.deltaY * 2.0f));
        if (onSelect) onSelect (n);
        repaint();
    }
    Rectangle<float> area() const { return getLocalBounds().toFloat().reduced (14, 12).withTrimmedRight (30).withTrimmedBottom (16); }
private:
    static float yOf (float db, Rectangle<float> a) { return a.getCentreY() - a.getHeight() * 0.5f * jlimit (-1.1f, 1.1f, db / 20.0f); }
    static float dbOf (float y, Rectangle<float> a) { return (a.getCentreY() - y) / (a.getHeight() * 0.5f) * 20.0f; }
    Point<float> node (int b, Rectangle<float> a) const
    {
        auto& bb = proc.mixLab.band[(size_t) b];
        const int t = bb.type.load();
        const float g = (t == kk::eqLowCut || t == kk::eqHighCut || t == kk::eqNotch) ? 0.0f : bb.gain.load();
        return { mixX (bb.freq.load(), a), yOf (g, a) };
    }
    int nodeAt (Point<float> p) const
    {
        const auto a = area(); int best = -1; float bd = 15.0f;
        for (int b = 0; b < kk::MixLabState::numBands; ++b) { const float d = node (b, a).getDistanceFrom (p); if (d < bd) { bd = d; best = b; } }
        return best;
    }
    void bandMenu (int b)
    {
        auto& bb = proc.mixLab.band[(size_t) b];
        PopupMenu m;
        m.addSectionHeader ("BAND " + String (b + 1));
        for (int t = 0; t < kk::numEqTypes; ++t) m.addItem (1 + t, kk::eqTypeName (t), true, bb.type.load() == t);
        m.addSeparator();
        m.addItem (20, "Slope 24 dB / oct (cuts)", true, bb.slope.load() == 2);
        m.addItem (21, "DYNAMIC (cuts only when it gets loud)", true, bb.dyn.load() > 0.001f);
        m.addItem (22, bb.on.load() ? "Off" : "On");
        m.showMenuAsync (PopupMenu::Options(), [this, b, safe = SafePointer<EqDisplay> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto& x = proc.mixLab.band[(size_t) b];
            if (r >= 1 && r <= kk::numEqTypes) { x.type = r - 1; x.on = true; }
            if (r == 20) x.slope = x.slope.load() == 2 ? 1 : 2;
            if (r == 21) x.dyn = x.dyn.load() > 0.001f ? 0.0f : 0.5f;
            if (r == 22) x.on = ! x.on.load();
            if (onSelect) onSelect (b);
            repaint();
        });
    }
    KeysKillaProcessor& proc; MixAnalyzer& an;
    Point<float> hover { -1, -1 };
    int drag = -1;
};

// ---------------- PUNCH COMP display: level history + gain reduction + the transfer curve ----------------
class CompDisplay : public Component, public SettableTooltipClient
{
public:
    explicit CompDisplay (KeysKillaProcessor& p) : proc (p) { setTooltip ("Grey = what comes in, white = what goes out, red = how much the compressor pulls down.  Drag the line in the curve box = threshold."); }
    void push()
    {
        auto& m = proc.mixLab;
        hist[(size_t) pos] = { 20.0f * std::log10 (m.mIn.load() + 1e-6f), 20.0f * std::log10 (m.mOut.load() + 1e-6f), m.mGr.load() };
        pos = (pos + 1) % (int) hist.size();
        m.mIn = 0; m.mOut = 0;
        repaint();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto r = getLocalBounds().toFloat();
        kk::modern::well (g, r, 12.0f);
        g.setColour (Colour (0xff0b0d12).withAlpha (t.night ? 0.55f : 0.82f)); g.fillRoundedRectangle (r.reduced (2), 11.0f);
        auto inner = r.reduced (14, 12);
        auto box = inner.removeFromRight (inner.getHeight()).reduced (4);
        inner.removeFromRight (14);
        const auto& m = proc.mixLab;
        // history
        auto yL = [&] (float db) { return inner.getBottom() - inner.getHeight() * jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };
        for (int db = -48; db <= 0; db += 12) { g.setColour (Colours::white.withAlpha (0.06f)); g.fillRect (inner.getX(), yL ((float) db), inner.getWidth(), 1.0f); g.setColour (Colours::white.withAlpha (0.35f)); g.setFont (kk::modern::font (10.0f, true, 0.0f)); g.drawText (String (db), (int) inner.getX() + 2, (int) yL ((float) db) - 12, 30, 11, Justification::left); }
        const int N = (int) hist.size();
        Path pin, pout, pgr;
        for (int i = 0; i < N; ++i)
        {
            const auto& h = hist[(size_t) ((pos + i) % N)];
            const float x = inner.getX() + inner.getWidth() * (float) i / (float) (N - 1);
            if (i == 0) { pin.startNewSubPath (x, inner.getBottom()); pout.startNewSubPath (x, yL (h.out)); pgr.startNewSubPath (x, inner.getY()); }
            pin.lineTo (x, yL (h.in)); pout.lineTo (x, yL (h.out));
            pgr.lineTo (x, inner.getY() + inner.getHeight() * jlimit (0.0f, 1.0f, -h.gr / 24.0f));
        }
        pin.lineTo (inner.getRight(), inner.getBottom()); pin.closeSubPath();
        pgr.lineTo (inner.getRight(), inner.getY()); pgr.closeSubPath();
        g.setColour (Colours::white.withAlpha (0.16f)); g.fillPath (pin);
        g.setColour (Colours::white.withAlpha (0.9f)); g.strokePath (pout, PathStrokeType (1.4f));
        g.setGradientFill (ColourGradient (Colour (0xffff3b5c).withAlpha (0.75f), 0, inner.getY(), Colour (0xffff8a3d).withAlpha (0.25f), 0, inner.getY() + inner.getHeight() * 0.4f, false));
        g.fillPath (pgr);
        const float ty = yL (m.thresh.load());
        g.setColour (Colour (0xffffd23f).withAlpha (0.7f)); g.fillRect (inner.getX(), ty, inner.getWidth(), 1.2f);
        // transfer curve
        g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (box, 6);
        auto bx = [&] (float db) { return box.getX() + box.getWidth() * jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };
        auto by = [&] (float db) { return box.getBottom() - box.getHeight() * jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };
        g.setColour (Colours::white.withAlpha (0.12f)); g.drawLine (box.getX(), box.getBottom(), box.getRight(), box.getY(), 1.0f);
        const float T = m.thresh.load(), Rt = jmax (1.0f, m.ratio.load()), W = m.knee.load();
        Path tc;
        for (int i = 0; i <= 60; ++i)
        {
            const float in = -60.0f + (float) i, over = in - T;
            float out;
            if (2 * over < -W) out = in; else if (W > 0 && 2 * std::abs (over) <= W) out = in + (1.0f / Rt - 1.0f) * (over + W / 2) * (over + W / 2) / (2 * W); else out = T + over / Rt;
            if (i == 0) tc.startNewSubPath (bx (in), by (out)); else tc.lineTo (bx (in), by (out));
        }
        g.setColour (Colour (0xffffd23f)); g.strokePath (tc, PathStrokeType (2.2f));
        const float inNow = hist[(size_t) ((pos - 1 + N) % N)].in;
        g.setColour (Colours::white); g.fillEllipse (bx (inNow) - 4, by (inNow + hist[(size_t) ((pos - 1 + N) % N)].gr) - 4, 8, 8);
        g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText ("THRESHOLD " + String (T, 1) + " dB", box.toNearestInt().removeFromTop (18), Justification::centred);
        // GR number
        g.setColour (Colour (0xffff5a6d)); g.setFont (kk::modern::font (26.0f, true, 0.02f));
        g.drawText (String (m.mGr.load(), 1) + " dB", inner.toNearestInt().removeFromTop (34).removeFromRight (140), Justification::centredRight);
        boxR = box;
    }
    void mouseDown (const MouseEvent& e) override { dragT = boxR.contains (e.position); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! dragT) return;
        proc.mixLab.thresh = jlimit (-60.0f, 0.0f, (boxR.getBottom() - e.position.y) / boxR.getHeight() * 60.0f - 60.0f);
        proc.mixLab.compOn = true;
        if (onChange) onChange();
        repaint();
    }
    std::function<void()> onChange;
private:
    struct H { float in = -60, out = -60, gr = 0; };
    KeysKillaProcessor& proc;
    std::array<H, 240> hist {};
    int pos = 0; Rectangle<float> boxR; bool dragT = false;
};

// ---------------- TIME MACHINE: the ERA timeline ----------------
class EraTimeline : public Component, public SettableTooltipClient
{
public:
    explicit EraTimeline (KeysKillaProcessor& p) : proc (p) { setTooltip ("ERA: drag through a century of machines - shellac, tape, vinyl, the old sampler, cassette ... clean ... the future.  All six modules follow."); }
    std::function<void()> onChange;
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (52, 4);
        const auto& e = kk::eras();
        auto track = r.withTrimmedTop (r.getHeight() * 0.42f).withHeight (10.0f);
        ColourGradient cg (Colour (0xffb07a4a), track.getX(), 0, Colour (0xffb04dff), track.getRight(), 0, false);
        cg.addColour (0.3, Colour (0xffff8a3d)); cg.addColour (0.55, Colour (0xff22d3ee)); cg.addColour (0.72, Colour (0xffeef0f2)); cg.addColour (0.86, Colour (0xff4d7dff));
        g.setGradientFill (cg); g.fillRoundedRectangle (track, 5);
        const float pos = proc.mixLab.era.load();
        for (int i = 0; i < (int) e.size(); ++i)
        {
            const float x = track.getX() + track.getWidth() * (float) i / (float) (e.size() - 1);
            const bool near = std::abs (pos * (float) (e.size() - 1) - (float) i) < 0.5f;
            g.setColour (Colours::white.withAlpha (near ? 1.0f : 0.5f)); g.fillEllipse (x - 4, track.getCentreY() - 4, 8, 8);
            g.setFont (kk::modern::font (near ? 13.0f : 11.0f, true, 0.12f));
            g.setColour (near ? kk::accentText() : kk::theme().text.withAlpha (0.75f));
            g.drawText (e[(size_t) i].name, (int) x - 50, (int) r.getY(), 100, 16, Justification::centred);
            g.setColour (kk::theme().dim); g.setFont (kk::modern::font (10.5f, true, 0.05f));
            g.drawText (String ((int) e[(size_t) i].year), (int) x - 40, (int) track.getBottom() + 4, 80, 14, Justification::centred);
        }
        const float hx = track.getX() + track.getWidth() * pos;
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillEllipse (hx - 12, track.getCentreY() - 11, 24, 24);
        g.setColour (Colours::white); g.fillEllipse (hx - 10, track.getCentreY() - 10, 20, 20);
        g.setColour (kk::theme().accent); g.drawEllipse (hx - 10, track.getCentreY() - 10, 20, 20, 2.5f);
        trackR = track;
    }
    void mouseDown (const MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (trackR.getWidth() <= 0) return;
        kk::applyEra (proc.mixLab, jlimit (0.0f, 1.0f, (e.position.x - trackR.getX()) / trackR.getWidth()));
        proc.mixLab.tmOn = true;
        if (onChange) onChange();
        repaint();
    }
private:
    KeysKillaProcessor& proc; Rectangle<float> trackR;
};

// one module card: colour, power, a big knob, a small knob, a little living picture
class TmCard : public Component
{
public:
    TmCard (KeysKillaProcessor& p, int m) : proc (p), mod (m)
    {
        static const uint32 cols[kk::numTm][2] { { 0xffff8a3d, 0xffffd23f }, { 0xff22d3ee, 0xff4d7dff }, { 0xffff3b5c, 0xffff8a3d }, { 0xff36ff6a, 0xff22d3ee }, { 0xff4d7dff, 0xffa78bfa }, { 0xffa78bfa, 0xffff4fd8 } };
        ca = Colour (cols[m][0]); cb = Colour (cols[m][1]);
        big = std::make_unique<RackKnob> (proc.mixLab.tmAmt[(size_t) m], "AMOUNT", 0, 1, 0.4, ca, cb);
        big->onValueChange = [this] { proc.mixLab.tmAmt[(size_t) mod] = (float) big->getValue(); if (big->getValue() > 0.01) { proc.mixLab.tmModOn[(size_t) mod] = true; proc.mixLab.tmOn = true; } repaint(); };
        addAndMakeVisible (*big);
        std::atomic<float>* sm = nullptr; const char* sn = "";
        switch (m)
        {
            case kk::tmWobble:  sm = &proc.mixLab.wobbleRate; sn = "RATE"; break;
            case kk::tmDistort: sm = &proc.mixLab.distTone; sn = "TONE"; break;
            case kk::tmDigital: sm = &proc.mixLab.digRate; sn = "RATE"; break;
            case kk::tmSpace:   sm = &proc.mixLab.spaceSize; sn = "SIZE"; break;
            case kk::tmFade:    sm = &proc.mixLab.fadeRate; sn = "OFTEN"; break;
            default: break;
        }
        if (sm != nullptr) { small = std::make_unique<RackKnob> (*sm, sn, 0, 1, 0.5, ca, cb); addAndMakeVisible (*small); }
        else
            for (int i = 0; i < 3; ++i)
            {
                static const char* nt[] { "VINYL", "TAPE", "HUM" };
                auto b = std::make_unique<TextButton> (nt[i]);
                b->setClickingTogglesState (false);
                b->onClick = [this, i] { proc.mixLab.noiseType = i; proc.mixLab.tmModOn[(size_t) mod] = true; proc.mixLab.tmOn = true; sync(); };
                addAndMakeVisible (*b); noiseBtns.push_back (std::move (b));
            }
        static const char* tips[] { "NOISE: vinyl crackle, tape hiss or mains hum - it follows the music, silence stays silent",
                                    "WOBBLE: the tape speed drifts (wow) and trembles (flutter)",
                                    "DISTORT: warm tube / tape saturation - TONE = darker or brighter",
                                    "DIGITAL: fewer bits and a lower sample rate - the old sampler grit",
                                    "SPACE: a small old room to a big plate",
                                    "FADE: weak spots on the tape - the sound dips and goes dull for a moment" };
        tip = tips[m];
        sync();
    }
    String tip;
    void sync()
    {
        big->sync(); if (small) small->sync();
        for (int i = 0; i < (int) noiseBtns.size(); ++i) noiseBtns[(size_t) i]->setToggleState (proc.mixLab.noiseType.load() == i, dontSendNotification);
        for (auto& b : noiseBtns) b->setColour (TextButton::buttonOnColourId, ca.withAlpha (0.6f));
        repaint();
    }
    void tick() { phase += 0.05f; repaint (picture().toNearestInt()); }
    void paint (Graphics& g) override
    {
        const bool on = proc.mixLab.tmOn.load() && proc.mixLab.tmModOn[(size_t) mod].load();
        auto r = getLocalBounds().toFloat().reduced (3);
        g.setGradientFill (ColourGradient (ca.withAlpha (on ? 0.22f : 0.06f), r.getX(), r.getY(), cb.withAlpha (on ? 0.10f : 0.03f), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 12);
        g.setColour (on ? ca : kk::theme().text.withAlpha (0.2f)); g.drawRoundedRectangle (r, 12, on ? 1.8f : 1.0f);
        g.setColour (on ? ca : kk::theme().dim); g.setFont (kk::modern::font (17.0f, true, 0.18f));
        g.drawText (kk::tmName (mod), r.reduced (14, 8).removeFromTop (24), Justification::centredLeft);
        const auto pw = power();
        g.setColour (on ? ca : kk::theme().text.withAlpha (0.25f)); g.fillEllipse (pw);
        g.setColour (Colours::black.withAlpha (0.6f)); g.drawEllipse (pw.reduced (4), 1.6f);
        // living picture
        const auto pr = picture();
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (pr, 8);
        const float amt = on ? proc.mixLab.tmAmt[(size_t) mod].load() : 0.0f;
        Graphics::ScopedSaveState ss (g); g.reduceClipRegion (pr.toNearestInt());
        g.setColour (ca);
        Path p;
        const int N = 64;
        for (int i = 0; i <= N; ++i)
        {
            const float u = (float) i / (float) N, x = pr.getX() + pr.getWidth() * u;
            float y = std::sin (u * 12.0f + phase * 2.0f);
            switch (mod)
            {
                case kk::tmNoise: y = y * 0.6f + (std::sin (u * 517.0f + phase * 40.0f) * std::sin (u * 91.0f - phase * 13.0f)) * amt * 0.9f; break;
                case kk::tmWobble: y = std::sin (u * (12.0f + 4.0f * amt * std::sin (phase * 0.7f)) + phase * 2.0f + amt * 1.5f * std::sin (u * 3.0f + phase)); break;
                case kk::tmDistort: y = std::tanh (y * (1.0f + 6.0f * amt)) / std::tanh (1.0f + 6.0f * amt); break;
                case kk::tmDigital: { const float lv = 2.0f + (1.0f - amt) * 14.0f; y = std::round (std::sin (std::round (u * (64.0f - 50.0f * amt)) / (64.0f - 50.0f * amt) * 12.0f + phase * 2.0f) * lv) / lv; break; }
                case kk::tmSpace: y = y * std::exp (-u * (3.0f - 2.5f * amt)) + amt * 0.4f * std::sin (u * 37.0f + phase * 3.0f) * std::exp (-u); break;
                default: y *= 1.0f - amt * 0.7f * (0.5f + 0.5f * std::sin (u * 4.0f + phase * 1.3f)); break;
            }
            const float yy = pr.getCentreY() - y * pr.getHeight() * 0.38f;
            if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
        }
        g.setColour (ca.withAlpha (on ? 0.95f : 0.35f)); g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        r.removeFromTop (30);
        auto pic = r.removeFromTop (r.getHeight() / 3);
        juce::ignoreUnused (pic);
        r.removeFromTop (6);
        auto k = r;
        if (small) { big->setBounds (k.removeFromLeft (k.getWidth() * 3 / 5)); small->setBounds (k.reduced (2, k.getHeight() / 6)); }
        else
        {
            big->setBounds (k.removeFromLeft (k.getWidth() * 3 / 5));
            auto b = k.reduced (4, 4);
            const int h = jmin (28, b.getHeight() / 3 - 4);
            for (auto& nb : noiseBtns) { nb->setBounds (b.removeFromTop (h)); b.removeFromTop (4); }
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (power().expanded (6).contains (e.position))
        {
            auto& o = proc.mixLab.tmModOn[(size_t) mod];
            o = ! o.load(); if (o.load()) proc.mixLab.tmOn = true;
            repaint();
        }
    }
private:
    Rectangle<float> power() const { return { (float) getWidth() - 36, 12, 20, 20 }; }
    Rectangle<float> picture() const { auto r = getLocalBounds().reduced (12).toFloat(); r.removeFromTop (30); return r.removeFromTop (r.getHeight() / 3); }
    KeysKillaProcessor& proc; int mod;
    Colour ca, cb;
    std::unique_ptr<RackKnob> big, small;
    std::vector<std::unique_ptr<TextButton>> noiseBtns;
    float phase = 0;
};

// ---------------- SPACE + ECHO view ----------------
class SpaceView : public Component
{
public:
    SpaceView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto& m = proc.mixLab;
        auto chipRow = [this] (std::vector<std::unique_ptr<HotButton>>& v, int count, std::function<const char* (int)> name, std::function<void (int)> fn, std::function<String (int)> tip)
        {
            for (int i = 0; i < count; ++i)
            {
                auto b = std::make_unique<HotButton> (lnf, name (i)); b->framed = true; b->setTooltip (tip (i));
                b->onClick = [this, i, fn] { fn (i); sync(); };
                addAndMakeVisible (*b); v.push_back (std::move (b));
            }
        };
        chipRow (spModes, kk::numSpaceModes, [] (int i) { return kk::spaceModeName (i); }, [this] (int i) { proc.mixLab.spMode = i; proc.mixLab.spOn = true; },
                 [] (int i) { static const char* t[] { "ROOM: a small real room - glue, not wash", "PLATE: bright, dense, smooth - the classic on vocals and leads",
                                                       "HALL: big and warm", "CLOUD: a huge, endless, moving space (up to 30 s)", "SHIMMER: the tail rises an octave - angelic pads from anything" }; return String (t[i]); });
        chipRow (dlModes, kk::numEchoModes, [] (int i) { return kk::echoModeName (i); }, [this] (int i) { proc.mixLab.dlMode = i; proc.mixLab.dlOn = true; },
                 [] (int i) { static const char* t[] { "CLEAN: exact digital echoes", "TAPE: echoes that wobble and get warmer", "PING-PONG: left, right, left ...", "LO-FI: crunchy, narrow, old sampler echoes" }; return String (t[i]); });
        chipRow (dlTimes, 6, [] (int i) { return kk::echoTimeName (i); }, [this] (int i) { proc.mixLab.dlTime = i; proc.mixLab.dlOn = true; }, [] (int) { return String ("The echo time, in time with FL's tempo"); });
        const Colour b1 (0xff4d7dff), b2 (0xffa78bfa), c1 (0xff22d3ee), c2 (0xff36ff6a);
        auto k = [this] (std::atomic<float>& t, const char* n, double lo, double hi, double def, Colour a, Colour b, std::function<String (double)> txt, std::vector<std::unique_ptr<RackKnob>>& into)
        { auto kn = std::make_unique<RackKnob> (t, n, lo, hi, def, a, b); kn->valueText = std::move (txt); addAndMakeVisible (*kn); into.push_back (std::move (kn)); };
        auto pct = [] (double v) { return String (roundToInt (v * 100)) + " %"; };
        k (m.spMix, "MIX", 0, 1, 0.25, b1, b2, pct, spKnobs);
        k (m.spDecay, "DECAY", 0, 1, 0.5, b1, b2, pct, spKnobs);
        k (m.spPre, "PRE-DELAY", 0, 240, 20, b1, b2, [] (double v) { return String (roundToInt (v)) + " ms"; }, spKnobs);
        k (m.spTone, "TONE", 0, 1, 0.6, b1, b2, [] (double v) { return v < 0.33 ? String ("DARK") : v < 0.66 ? String ("WARM") : String ("BRIGHT"); }, spKnobs);
        k (m.spMod, "MOTION", 0, 1, 0.4, b1, b2, pct, spKnobs);
        k (m.spWidth, "WIDTH", 0, 1, 1, b1, b2, pct, spKnobs);
        k (m.spDuck, "DUCK", 0, 1, 0, b1, b2, pct, spKnobs);
        k (m.dlMix, "MIX", 0, 1, 0.25, c1, c2, pct, dlKnobs);
        k (m.dlFb, "FEEDBACK", 0, 0.95, 0.4, c1, c2, pct, dlKnobs);
        k (m.dlTone, "TONE", 0, 1, 0.6, c1, c2, [] (double v) { return v < 0.33 ? String ("DARK") : v < 0.66 ? String ("WARM") : String ("BRIGHT"); }, dlKnobs);
        k (m.dlDuck, "DUCK", 0, 1, 0, c1, c2, pct, dlKnobs);
        for (int i = 0; i < (int) spKnobs.size(); ++i) { auto* kn = spKnobs[(size_t) i].get(); auto* tgt = spTargets()[(size_t) i]; kn->onValueChange = [this, kn, tgt] { *tgt = (float) kn->getValue(); proc.mixLab.spOn = true; sync(); }; }
        for (int i = 0; i < (int) dlKnobs.size(); ++i) { auto* kn = dlKnobs[(size_t) i].get(); auto* tgt = dlTargets()[(size_t) i]; kn->onValueChange = [this, kn, tgt] { *tgt = (float) kn->getValue(); proc.mixLab.dlOn = true; sync(); }; }
        spKnobs[6]->setTooltip ("DUCK: the reverb steps back while the sound plays and blooms in the gaps - big space, clear mix");
        dlKnobs[3]->setTooltip ("DUCK: the echoes wait for the gaps - they never smear the melody");
        auto hb = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        hb (spPow, "ON", "SPACE on / off", [this] { proc.mixLab.spOn = ! proc.mixLab.spOn.load(); sync(); });
        hb (dlPow, "ON", "ECHO on / off", [this] { proc.mixLab.dlOn = ! proc.mixLab.dlOn.load(); sync(); });
        hb (freezeBtn, "FREEZE", "FREEZE: the space holds what it has forever - play over a frozen pad", [this] { proc.mixLab.spFreeze = ! proc.mixLab.spFreeze.load(); proc.mixLab.spOn = true; sync(); });
        hb (diceBtn, "DICE", "A random space and echo (in good taste)", [this]
        {
            Random r; auto& mm = proc.mixLab;
            mm.spMode = r.nextInt (kk::numSpaceModes); mm.spMix = 0.12f + 0.3f * r.nextFloat(); mm.spDecay = r.nextFloat(); mm.spPre = 5.0f + 60.0f * r.nextFloat(); mm.spTone = 0.3f + 0.6f * r.nextFloat(); mm.spMod = r.nextFloat(); mm.spDuck = r.nextFloat() < 0.5f ? 0.0f : 0.5f;
            mm.dlMode = r.nextInt (kk::numEchoModes); mm.dlTime = r.nextInt (6); mm.dlMix = 0.1f + 0.25f * r.nextFloat(); mm.dlFb = 0.2f + 0.5f * r.nextFloat(); mm.dlDuck = 0.5f * r.nextFloat();
            mm.spOn = true; mm.dlOn = r.nextFloat() < 0.7f;
            sync();
        });
        sync();
    }
    void sync()
    {
        auto& m = proc.mixLab;
        for (int i = 0; i < (int) spModes.size(); ++i) { spModes[(size_t) i]->selected = m.spMode.load() == i; spModes[(size_t) i]->repaint(); }
        for (int i = 0; i < (int) dlModes.size(); ++i) { dlModes[(size_t) i]->selected = m.dlMode.load() == i; dlModes[(size_t) i]->repaint(); }
        for (int i = 0; i < (int) dlTimes.size(); ++i) { dlTimes[(size_t) i]->selected = m.dlTime.load() == i; dlTimes[(size_t) i]->repaint(); }
        for (auto& kn : spKnobs) kn->sync();
        for (auto& kn : dlKnobs) kn->sync();
        spPow.selected = m.spOn.load(); dlPow.selected = m.dlOn.load(); freezeBtn.selected = m.spFreeze.load();
        for (auto* b : { &spPow, &dlPow, &freezeBtn }) b->repaint();
        repaint();
    }
    void tick() { phase += 1.0f / 30.0f; repaint (spPic.getUnion (dlPic)); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto& m = proc.mixLab;
        auto panel = [&] (Rectangle<int> r, Colour a, Colour b, const String& name, bool on)
        {
            g.setGradientFill (ColourGradient (a.withAlpha (on ? 0.16f : 0.05f), (float) r.getX(), (float) r.getY(), b.withAlpha (on ? 0.08f : 0.02f), (float) r.getRight(), (float) r.getBottom(), false));
            g.fillRoundedRectangle (r.toFloat(), 12);
            g.setColour (on ? a : t.text.withAlpha (0.2f)); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 12, on ? 1.6f : 1.0f);
            g.setColour (on ? a : t.dim); g.setFont (kk::modern::font (20.0f, true, 0.2f));
            g.drawText (name, r.getX() + 16, r.getY() + 10, 200, 28, Justification::centredLeft);
        };
        panel (spR, Colour (0xff4d7dff), Colour (0xffa78bfa), "SPACE", m.spOn.load());
        panel (dlR, Colour (0xff22d3ee), Colour (0xff36ff6a), "ECHO", m.dlOn.load());
        // SPACE picture: a burst that blooms and decays (longer = bigger), shimmer rises, freeze holds
        {
            auto r = spPic.toFloat();
            g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r, 10);
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (spPic);
            const int mode = m.spMode.load(); const float dec = m.spDecay.load(), mix = m.spOn.load() ? m.spMix.load() : 0.05f;
            const float len = (mode == kk::spRoom ? 0.15f : mode == kk::spPlate ? 0.4f : mode == kk::spHall ? 0.6f : 1.0f) * (0.3f + 0.7f * dec);
            const float pre = m.spPre.load() / 240.0f * 0.15f;
            Random rr (7);
            const float cyc = std::fmod (phase * 0.4f, 1.0f);
            for (int i = 0; i < 260; ++i)
            {
                const float u = rr.nextFloat(), x = r.getX() + r.getWidth() * (pre + u * 0.85f);
                const float env = m.spFreeze.load() ? 0.7f : std::exp (-u / std::max (0.03f, len) * 2.0f);
                const float live = std::max (0.0f, 1.0f - std::abs (u - cyc) * 6.0f);
                const float h = r.getHeight() * 0.45f * env * (0.4f + 0.6f * rr.nextFloat()) * (0.4f + 1.4f * mix);
                const float yOff = mode == kk::spShimmer ? -u * r.getHeight() * 0.25f : 0.0f;
                g.setColour (Colour (0xff4d7dff).interpolatedWith (Colour (0xffff4fd8), u).withAlpha (0.25f + 0.6f * live));
                g.fillRect (x, r.getCentreY() - h + yOff, 2.0f, h * 2.0f);
            }
            g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (12.0f, true, 0.1f));
            const float rt = mode == kk::spRoom ? 0.3f + 1.5f * dec : mode == kk::spPlate ? 0.6f + 4.0f * dec : mode == kk::spHall ? 1.0f + 6.0f * dec : mode == kk::spCloud ? 3.0f + 27.0f * dec : 2.0f + 12.0f * dec;
            g.drawText (String (kk::spaceModeName (mode)) + "   " + (m.spFreeze.load() ? String ("FROZEN") : String (rt, 1) + " s"), spPic.reduced (10, 6), Justification::topRight);
        }
        // ECHO picture: the taps, in time, fading by feedback
        {
            auto r = dlPic.toFloat();
            g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r, 10);
            const float beats = (float) kk::echoBeats (m.dlTime.load()), fb = m.dlFb.load();
            const int mode = m.dlMode.load();
            float a = 1.0f;
            for (int k2 = 0; k2 < 16 && a > 0.03f; ++k2)
            {
                const float x = r.getX() + 10 + (r.getWidth() - 20) * (float) k2 * beats / 4.0f;
                if (x > r.getRight() - 6) break;
                const float h = (r.getHeight() - 20) * a;
                const bool left = mode != kk::dlPingPong || k2 % 2 == 0;
                g.setColour (Colour (0xff22d3ee).interpolatedWith (Colour (0xff36ff6a), (float) k2 / 10.0f).withAlpha (k2 == 0 ? 0.9f : 0.75f));
                g.fillRoundedRectangle (x - 4, left ? r.getBottom() - 10 - h : r.getY() + 10, 8, h * (mode == kk::dlPingPong ? 0.5f : 1.0f), 3);
                a *= k2 == 0 ? std::max (0.3f, m.dlMix.load() * 2.0f) : fb;
            }
            g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (12.0f, true, 0.1f));
            g.drawText (String (kk::echoTimeName (m.dlTime.load())) + " at " + String (roundToInt (m.bpm.load())) + " BPM", dlPic.reduced (10, 6), Justification::topRight);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds();
        spR = r.removeFromLeft (r.getWidth() * 58 / 100).reduced (0, 0); r.removeFromLeft (12); dlR = r;
        {
            auto a = spR.reduced (14, 10);
            auto head = a.removeFromTop (34);
            spPow.setBounds (head.getX() + 110, head.getY() + 2, 48, 30); freezeBtn.setBounds (head.getRight() - 196, head.getY() + 2, 96, 30); diceBtn.setBounds (head.getRight() - 94, head.getY() + 2, 92, 30);
            a.removeFromTop (6);
            auto chips = a.removeFromTop (34);
            const int cw = chips.getWidth() / kk::numSpaceModes;
            for (auto& c : spModes) c->setBounds (chips.removeFromLeft (cw).reduced (3, 1));
            a.removeFromTop (10);
            auto knobs = a.removeFromBottom (110);
            spPic = a.reduced (0, 4);
            const int kw = knobs.getWidth() / (int) spKnobs.size();
            for (auto& kn : spKnobs) kn->setBounds (knobs.removeFromLeft (kw).reduced (2, 0));
        }
        {
            auto a = dlR.reduced (14, 10);
            auto head = a.removeFromTop (34);
            dlPow.setBounds (head.getX() + 100, head.getY() + 2, 48, 30);
            a.removeFromTop (6);
            auto chips = a.removeFromTop (34);
            const int cw = chips.getWidth() / kk::numEchoModes;
            for (auto& c : dlModes) c->setBounds (chips.removeFromLeft (cw).reduced (3, 1));
            a.removeFromTop (6);
            auto times = a.removeFromTop (32);
            const int tw = times.getWidth() / 6;
            for (auto& c : dlTimes) c->setBounds (times.removeFromLeft (tw).reduced (3, 1));
            a.removeFromTop (8);
            auto knobs = a.removeFromBottom (110);
            dlPic = a.reduced (0, 4);
            const int kw = knobs.getWidth() / (int) dlKnobs.size();
            for (auto& kn : dlKnobs) kn->setBounds (knobs.removeFromLeft (kw).reduced (2, 0));
        }
    }
private:
    std::vector<std::atomic<float>*> spTargets() { auto& m = proc.mixLab; return { &m.spMix, &m.spDecay, &m.spPre, &m.spTone, &m.spMod, &m.spWidth, &m.spDuck }; }
    std::vector<std::atomic<float>*> dlTargets() { auto& m = proc.mixLab; return { &m.dlMix, &m.dlFb, &m.dlTone, &m.dlDuck }; }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<HotButton>> spModes, dlModes, dlTimes;
    std::vector<std::unique_ptr<RackKnob>> spKnobs, dlKnobs;
    HotButton spPow { lnf }, dlPow { lnf }, freezeBtn { lnf }, diceBtn { lnf };
    Rectangle<int> spR, dlR, spPic, dlPic;
    float phase = 0;
};

struct PaintOverlay : public Component
{
    std::function<void (Graphics&)> paintFn; std::function<void (Point<int>)> move, up; std::function<void()> exit;
    void paint (Graphics& g) override { if (paintFn) paintFn (g); }
    void mouseMove (const MouseEvent& e) override { if (move) move (e.getPosition()); }
    void mouseUp (const MouseEvent& e) override { if (up) up (e.getPosition()); }
    void mouseExit (const MouseEvent&) override { if (exit) exit(); }
};

// ---------------- the page ----------------
class MixLabPage : public Component, private Timer
{
public:
    MixLabPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l), eqView (p, an), compView (p), era (p), spaceView (p, l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (tabEq, "SHAPE EQ", "SHAPE EQ: 8 bands, live analyzer, DYNAMIC bands, AUTO and EVOLVE", [this] { setView (0); });
        btn (tabComp, "PUNCH COMP", "PUNCH COMP: five characters, soft knee, parallel mix", [this] { setView (1); });
        btn (tabSp, "SPACE", "SPACE: room, plate, hall, an endless CLOUD, SHIMMER - and tempo ECHO (clean, tape, ping-pong, lo-fi), both can DUCK", [this] { setView (3); });
        addChildComponent (spaceView);
        btn (tabTm, "TIME MACHINE", "TIME MACHINE: noise, wobble, distortion, digital grit, space and tape fades - and the ERA timeline", [this] { setView (2); });
        for (auto* b : { &powEq, &powComp, &powTm })
        {
            b->framed = true; b->setButtonText ("ON"); addAndMakeVisible (*b);
        }
        powEq.onClick = [this] { proc.mixLab.eqOn = ! proc.mixLab.eqOn.load(); refresh(); };
        powComp.onClick = [this] { proc.mixLab.compOn = ! proc.mixLab.compOn.load(); refresh(); };
        powTm.onClick = [this] { proc.mixLab.tmOn = ! proc.mixLab.tmOn.load(); refresh(); };
        powEq.setTooltip ("SHAPE EQ on / off"); powComp.setTooltip ("PUNCH COMP on / off"); powTm.setTooltip ("TIME MACHINE on / off");

        // ---- EQ
        addChildComponent (eqView);
        eqView.onSelect = [this] (int b) { selectBand (b); };
        addChildComponent (variantStrip);
        variantStrip.paintFn = [this] (Graphics& g) { paintVariants (g); };
        variantStrip.move = [this] (Point<int> p) { hoverAt (p); };
        variantStrip.exit = [this] { hoverAt ({ -1, -1 }); };
        variantStrip.up = [this] (Point<int> p) { pickAt (p); };
        auto ck = [this] (std::unique_ptr<RackKnob>& k, std::atomic<float>& t, const char* n, double lo, double hi, double def, Colour a, Colour b, std::function<String (double)> txt)
        { k = std::make_unique<RackKnob> (t, n, lo, hi, def, a, b); k->valueText = std::move (txt); addChildComponent (*k); };
        for (int i = 0; i < kk::numEqTypes; ++i) typeBox.addItem (kk::eqTypeName (i), i + 1);
        typeBox.onChange = [this] { proc.mixLab.band[(size_t) eqView.sel].type = typeBox.getSelectedId() - 1; proc.mixLab.band[(size_t) eqView.sel].on = true; eqView.repaint(); };
        typeBox.setTooltip ("The shape of this band");
        addChildComponent (typeBox);
        btn (bandOn, "BAND ON", "This band on / off", [this] { auto& b = proc.mixLab.band[(size_t) eqView.sel]; b.on = ! b.on.load(); selectBand (eqView.sel); });
        btn (dynBtn, "DYNAMIC", "DYNAMIC: the band cuts only when that range gets loud (a resonance that rings, a harsh note)", [this] { auto& b = proc.mixLab.band[(size_t) eqView.sel]; b.dyn = b.dyn.load() > 0.001f ? 0.0f : 0.5f; b.on = true; selectBand (eqView.sel); });
        btn (autoEqBtn, "AUTO", "AUTO: finds ringing frequencies and nudges the tone toward the COACH's style - a starting point for your ears", [this] { refreshReading(); note = "AUTO: " + kk::coach::autoEq (proc.mixLab, reading, proc.coachGenre); selectBand (eqView.sel); repaint(); });
        btn (evolveEqBtn, "EVOLVE", "EVOLVE: four new curves grow from yours.  Hover = hear it (level matched), click = keep it.", [this] { startEvolve(); });
        btn (resetEqBtn, "RESET", "All bands flat", [this] { proc.mixLab.resetEq(); selectBand (eqView.sel); });
        for (auto* b : { &bandOn, &dynBtn, &autoEqBtn, &evolveEqBtn, &resetEqBtn }) b->setVisible (false);
        autoEqBtn.hero = true;

        // ---- COMP
        addChildComponent (compView);
        compView.onChange = [this] { syncKnobs(); };
        for (int s = 0; s < kk::numCompStyles; ++s)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::compStyleName (s)); b->framed = true;
            static const char* tips[] { "CLEAN: transparent, does what the knobs say", "PUNCH: a slower attack lets the hit through, then it grabs - snap",
                                        "GLUE: slow and smooth on the whole bus - the parts become one record", "OPTO: like a light cell - quick at first, then it lets go slowly (vocals, pads)",
                                        "VINTAGE: fast and coloured, it adds harmonics - aggressive" };
            b->setTooltip (tips[s]);
            b->onClick = [this, s] { compStyle (s); };
            addChildComponent (*b); styleBtns.push_back (std::move (b));
        }
        btn (autoGainBtn, "AUTO GAIN", "Makes up the level the compressor takes away (a fair comparison)", [this] { proc.mixLab.autoGain = ! proc.mixLab.autoGain.load(); refresh(); });
        btn (scBtn, "BASS SAFE", "The compressor does not listen to the deep bass - the 808 does not make the melody pump", [this] { proc.mixLab.scHp = ! proc.mixLab.scHp.load(); refresh(); });
        btn (autoCompBtn, "AUTO", "AUTO: sets the threshold so the compressor works about 3-4 dB on what plays now", [this]
        {
            const float rms = 20.0f * std::log10 (proc.mixLab.mRms.load() + 1e-9f);
            if (rms < -60) { note = "play something first"; repaint(); return; }
            proc.mixLab.thresh = jlimit (-50.0f, -3.0f, rms + 2.0f); proc.mixLab.compOn = true; note = "threshold " + String (proc.mixLab.thresh.load(), 1) + " dB"; syncKnobs(); refresh();
        });
        for (auto* b : { &autoGainBtn, &scBtn, &autoCompBtn }) b->setVisible (false);
        autoCompBtn.hero = true;
        const Colour y1 (0xffffd23f), y2 (0xffff8a3d), r1 (0xffff3b5c), c1 (0xff22d3ee), c2 (0xff4d7dff);
        ck (kThresh, proc.mixLab.thresh, "THRESHOLD", -60, 0, -18, y1, y2, [] (double v) { return String (v, 1) + " dB"; });
        ck (kRatio, proc.mixLab.ratio, "RATIO", 1, 20, 3, y1, y2, [] (double v) { return String (v, 1) + " : 1"; });
        ck (kAttack, proc.mixLab.attack, "ATTACK", 0.1, 100, 15, c1, c2, [] (double v) { return String (v, v < 10 ? 1 : 0) + " ms"; });
        ck (kRelease, proc.mixLab.release, "RELEASE", 10, 1200, 120, c1, c2, [] (double v) { return String (roundToInt (v)) + " ms"; });
        ck (kKnee, proc.mixLab.knee, "KNEE", 0, 24, 6, c1, c2, [] (double v) { return String (v, 1) + " dB"; });
        ck (kMix, proc.mixLab.compMix, "MIX", 0, 1, 1, r1, y2, [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        ck (kCompOut, proc.mixLab.compOut, "OUTPUT", -12, 12, 0, r1, y2, [] (double v) { return String (v, 1) + " dB"; });
        kRatio->setSkewFactorFromMidPoint (4.0); kAttack->setSkewFactorFromMidPoint (10.0); kRelease->setSkewFactorFromMidPoint (150.0);
        // EQ band knobs (they re-target the selected band)
        ck (kFreq, dummy[0], "FREQ", 20, 20000, 1000, c1, c2, [] (double v) { return kk::coach::hzText ((float) v); });
        ck (kGain, dummy[1], "GAIN", -18, 18, 0, y1, y2, [] (double v) { return String (v, 1) + " dB"; });
        ck (kQ, dummy[2], "Q", 0.1, 24, 0.9, c1, c2, [] (double v) { return String (v, 2); });
        ck (kDyn, dummy[3], "DYNAMIC", 0, 1, 0, r1, y2, [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        ck (kEqOut, proc.mixLab.eqOut, "OUTPUT", -12, 12, 0, y1, y2, [] (double v) { return String (v, 1) + " dB"; });
        kFreq->setSkewFactorFromMidPoint (1000.0); kQ->setSkewFactorFromMidPoint (1.5);
        kFreq->onValueChange = [this] { proc.mixLab.band[(size_t) eqView.sel].freq = (float) kFreq->getValue(); eqView.repaint(); };
        kGain->onValueChange = [this] { proc.mixLab.band[(size_t) eqView.sel].gain = (float) kGain->getValue(); proc.mixLab.band[(size_t) eqView.sel].on = true; eqView.repaint(); };
        kQ->onValueChange = [this] { proc.mixLab.band[(size_t) eqView.sel].q = (float) kQ->getValue(); eqView.repaint(); };
        kDyn->onValueChange = [this] { proc.mixLab.band[(size_t) eqView.sel].dyn = (float) kDyn->getValue(); eqView.repaint(); repaintDynBtn(); };

        // ---- TIME MACHINE
        addChildComponent (era);
        era.onChange = [this] { for (auto& c : cards) c->sync(); refresh(); };
        for (int m = 0; m < kk::numTm; ++m) { cards.push_back (std::make_unique<TmCard> (proc, m)); addChildComponent (*cards.back()); }
        ck (kMag, proc.mixLab.magnitude, "MAGNITUDE", 0, 1.5, 1, Colour (0xffff8a3d), Colour (0xffb04dff), [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        ck (kTmMix, proc.mixLab.tmMix, "MIX", 0, 1, 1, Colour (0xff22d3ee), Colour (0xffb04dff), [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        btn (diceTm, "DICE", "A random machine: a random era and random amounts", [this]
        {
            Random rnd;
            kk::applyEra (proc.mixLab, rnd.nextFloat());
            for (int m = 0; m < kk::numTm; ++m) if (rnd.nextFloat() < 0.35f) { proc.mixLab.tmModOn[(size_t) m] = ! proc.mixLab.tmModOn[(size_t) m].load(); proc.mixLab.tmAmt[(size_t) m] = 0.15f + 0.6f * rnd.nextFloat(); }
            proc.mixLab.tmOn = true;
            for (auto& c : cards) c->sync();
            refresh();
        });
        diceTm.setVisible (false);

        // ---- COACH
        for (int gi = -1; gi < kk::mel::numGenres; ++gi) styleBox.addItem (gi < 0 ? String ("ANY STYLE") : String (kk::mel::genreName (gi)), gi + 2);
        styleBox.onChange = [this] { proc.coachGenre = styleBox.getSelectedId() - 2; refreshReading(); repaint(); };
        styleBox.setTooltip ("What the COACH compares your sound with");
        addAndMakeVisible (styleBox);
        btn (modeSound, "MY SOUND", "The COACH judges one melodic sound (it should leave the low end to the 808)", [this] { coachMode = 0; refresh(); refreshReading(); });
        btn (modeBeat, "WHOLE BEAT", "The COACH judges a whole beat (EVOLVE on FL's master as an effect)", [this] { coachMode = 1; refresh(); refreshReading(); });
        for (int i = 0; i < 4; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, "FIX"); b->framed = true; b->hero = true;
            b->onClick = [this, i] { applyTip (i); };
            addChildComponent (*b); fixBtns.push_back (std::move (b));
        }
        setView (0);
        selectBand (eqView.sel);
        startTimerHz (30);
    }
    void visibilityChanged() override { if (isVisible()) { syncKnobs(); refresh(); } }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("MIX LAB", 24, 12, 200, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("the last stage of every sound - and of your whole beat when EVOLVE sits on FL's master as an effect", 200, 18, mainArea().getRight() - 200, 28, Justification::centredLeft);
        // panel behind the module
        kk::modern::plate (g, mainArea().toFloat(), 16.0f);
        if (view == 0)
        {
            auto br = bandStrip().toFloat();
            g.setColour (t.text); g.setFont (kk::modern::font (12.0f, true, 0.2f));
            g.drawText ("BAND " + String (eqView.sel + 1), br.removeFromLeft (80).toNearestInt(), Justification::centredLeft);
            for (int b = 0; b < kk::MixLabState::numBands; ++b)
            {
                const auto c = chip (b).toFloat();
                const bool on = proc.mixLab.band[(size_t) b].on.load();
                g.setColour (bandColour (b).withAlpha (on ? 0.9f : 0.2f)); g.fillRoundedRectangle (c, 6);
                if (b == eqView.sel) { g.setColour (t.text); g.drawRoundedRectangle (c.expanded (2), 7, 1.6f); }
                g.setColour (on ? Colours::black : t.text); g.setFont (kk::modern::font (12.0f, true, 0.0f));
                g.drawText (String (b + 1), c.toNearestInt(), Justification::centred);
            }
        }
        if (view == 2)
        {
            g.setColour (t.text); g.setFont (kk::modern::font (12.0f, true, 0.25f));
            g.drawText ("ERA", mainArea().getX() + 18, era.getY() - 14, 60, 14, Justification::centredLeft);
            const auto& m = proc.mixLab;
            g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.06f));
            g.drawText ("BANDWIDTH " + kk::coach::hzText (m.tmHp.load()) + " - " + kk::coach::hzText (m.tmLp.load()) + (m.tmMono.load() > 0.05f ? "   MONO " + String (roundToInt (m.tmMono.load() * 100)) + "%" : String()),
                        kMag->getX() - 10, kMag->getBottom() + 2, kMag->getWidth() * 2 + 40, 16, Justification::centredLeft);
        }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.5f, true, 0.03f)); g.drawFittedText (note, Rectangle<int> (mainArea().getX() + 16, mainArea().getBottom() - 26, mainArea().getWidth() - 32, 20), Justification::centredLeft, 1, 0.8f); }
        paintCoach (g);
    }
    void resized() override
    {
        const auto M = mainArea();
        int x = M.getX() + 14;
        for (auto pr : { std::pair<HotButton*, HotButton*> { &tabEq, &powEq }, { &tabComp, &powComp }, { &tabTm, &powTm } })
        {
            pr.first->setBounds (x, M.getY() + 12, 150, 36); pr.second->setBounds (x + 152, M.getY() + 14, 44, 32); x += 206;
        }
        tabSp.setBounds (x, M.getY() + 12, 130, 36);
        const auto body = M.reduced (14).withTrimmedTop (50).withTrimmedBottom (26);
        // EQ
        {
            auto b = body;
            auto bottom = b.removeFromBottom (150);
            eqView.setBounds (b);
            variantStrip.setBounds (b.getX() + 12, b.getBottom() - 112, b.getWidth() - 60, 98);
            auto strip = bottom.removeFromTop (34);
            stripR = strip;
            auto ctrl = bottom.reduced (0, 6);
            typeBox.setBounds (ctrl.getX(), ctrl.getY() + 8, 140, 28);
            bandOn.setBounds (ctrl.getX(), ctrl.getY() + 42, 68, 30); dynBtn.setBounds (ctrl.getX() + 72, ctrl.getY() + 42, 68, 30);
            int kx = ctrl.getX() + 160;
            for (auto* k : { kFreq.get(), kGain.get(), kQ.get(), kDyn.get() }) { k->setBounds (kx, ctrl.getY(), 90, ctrl.getHeight()); kx += 96; }
            kEqOut->setBounds (ctrl.getRight() - 96, ctrl.getY(), 90, ctrl.getHeight());
            const int bx = kx + 20;
            autoEqBtn.setBounds (bx, ctrl.getY() + 6, 110, 34); evolveEqBtn.setBounds (bx + 116, ctrl.getY() + 6, 110, 34);
            resetEqBtn.setBounds (bx, ctrl.getY() + 46, 110, 30);
        }
        // COMP
        {
            auto b = body;
            auto bottom = b.removeFromBottom (170);
            compView.setBounds (b);
            auto chips = bottom.removeFromTop (40);
            const int cw = (chips.getWidth() - 3 * 120) / kk::numCompStyles;
            for (auto& sb : styleBtns) sb->setBounds (chips.removeFromLeft (cw).reduced (3, 2));
            autoGainBtn.setBounds (chips.removeFromLeft (120).reduced (3, 2)); scBtn.setBounds (chips.removeFromLeft (120).reduced (3, 2)); autoCompBtn.setBounds (chips.removeFromLeft (120).reduced (3, 2));
            bottom.removeFromTop (6);
            const int kw = bottom.getWidth() / 7;
            for (auto* k : { kThresh.get(), kRatio.get(), kAttack.get(), kRelease.get(), kKnee.get(), kMix.get(), kCompOut.get() }) k->setBounds (bottom.removeFromLeft (kw).reduced (8, 0));
        }
        // TIME MACHINE
        {
            auto b = body;
            auto top = b.removeFromTop (78);
            auto right = top.removeFromRight (330);
            era.setBounds (top.withTrimmedTop (14));
            kMag->setBounds (right.getX() + 10, right.getY(), 100, 72); kTmMix->setBounds (right.getX() + 116, right.getY(), 90, 72); diceTm.setBounds (right.getRight() - 100, right.getY() + 18, 90, 34);
            b.removeFromTop (24);
            const int cw = b.getWidth() / 3, ch = b.getHeight() / 2;
            for (int m = 0; m < kk::numTm; ++m) cards[(size_t) m]->setBounds (b.getX() + (m % 3) * cw, b.getY() + (m / 3) * ch, cw, ch);
        }
        spaceView.setBounds (body);
        // COACH
        const auto C = coachArea();
        styleBox.setBounds (C.getRight() - 172, C.getY() + 14, 158, 28);
        modeSound.setBounds (C.getX() + 16, C.getY() + 282, (C.getWidth() - 40) / 2, 30); modeBeat.setBounds (modeSound.getRight() + 8, C.getY() + 282, (C.getWidth() - 40) / 2, 30);
        layoutFixButtons();
    }
    void debugShow (int v) { setView (v); }
    void debugTick() { an.update (proc.mixLab, (float) proc.getSampleRate()); compView.push(); for (auto& c : cards) c->tick(); refreshReading(); repaint(); }
private:
    Rectangle<int> mainArea() const { return { 16, 62, getWidth() - 16 - 392, getHeight() - 78 }; }
    Rectangle<int> coachArea() const { return { getWidth() - 392 + 8, 62, 376, getHeight() - 78 }; }
    Rectangle<int> bandStrip() const { return stripR; }
    Rectangle<int> chip (int b) const { return { stripR.getX() + 84 + b * 40, stripR.getY() + 4, 34, 26 }; }
    Rectangle<int> tipRect (int i) const { const auto C = coachArea(); return { C.getX() + 12, C.getY() + 322 + i * 96, C.getWidth() - 24, 90 }; }
    int maxTips() const { const auto C = coachArea(); return jlimit (1, 4, (C.getBottom() - 80 - (C.getY() + 322)) / 96); }
    bool wholeBeat() const { return coachMode == 1 || (coachMode < 0 && proc.mixLab.inputBlocks.load() > 20); }

    void setView (int v)
    {
        view = v;
        eqView.setVisible (v == 0);
        for (Component* c : { (Component*) &typeBox, (Component*) &bandOn, (Component*) &dynBtn, (Component*) &autoEqBtn, (Component*) &evolveEqBtn, (Component*) &resetEqBtn,
                              (Component*) kFreq.get(), (Component*) kGain.get(), (Component*) kQ.get(), (Component*) kDyn.get(), (Component*) kEqOut.get() }) c->setVisible (v == 0);
        compView.setVisible (v == 1);
        for (auto& b : styleBtns) b->setVisible (v == 1);
        for (Component* c : { (Component*) &autoGainBtn, (Component*) &scBtn, (Component*) &autoCompBtn, (Component*) kThresh.get(), (Component*) kRatio.get(), (Component*) kAttack.get(),
                              (Component*) kRelease.get(), (Component*) kKnee.get(), (Component*) kMix.get(), (Component*) kCompOut.get() }) c->setVisible (v == 1);
        era.setVisible (v == 2);
        for (auto& c : cards) c->setVisible (v == 2);
        for (Component* c : { (Component*) kMag.get(), (Component*) kTmMix.get(), (Component*) &diceTm }) c->setVisible (v == 2);
        spaceView.setVisible (v == 3); if (v == 3) spaceView.sync();
        if (v != 0) stopEvolve (false);
        variantStrip.setVisible (v == 0 && ! variantsShown.empty());
        note = {};
        refresh();
    }
    void refresh()
    {
        tabEq.selected = view == 0; tabComp.selected = view == 1; tabTm.selected = view == 2; tabSp.selected = view == 3; tabSp.repaint();
        powEq.selected = proc.mixLab.eqOn.load(); powComp.selected = proc.mixLab.compOn.load(); powTm.selected = proc.mixLab.tmOn.load();
        for (int s = 0; s < (int) styleBtns.size(); ++s) styleBtns[(size_t) s]->selected = proc.mixLab.compStyle.load() == s;
        autoGainBtn.selected = proc.mixLab.autoGain.load(); scBtn.selected = proc.mixLab.scHp.load();
        styleBox.setSelectedId (proc.coachGenre + 2, dontSendNotification);
        modeSound.selected = ! wholeBeat(); modeBeat.selected = wholeBeat(); modeSound.repaint(); modeBeat.repaint();
        for (auto* b : { &tabEq, &tabComp, &tabTm, &powEq, &powComp, &powTm, &autoGainBtn, &scBtn }) b->repaint();
        for (auto& b : styleBtns) b->repaint();
        repaint();
    }
    void syncKnobs()
    {
        for (auto* k : { kThresh.get(), kRatio.get(), kAttack.get(), kRelease.get(), kKnee.get(), kMix.get(), kCompOut.get(), kEqOut.get(), kMag.get(), kTmMix.get() }) k->sync();
        for (auto& c : cards) c->sync();
        selectBand (eqView.sel);
    }
    void selectBand (int b)
    {
        eqView.sel = b;
        auto& bb = proc.mixLab.band[(size_t) b];
        typeBox.setSelectedId (bb.type.load() + 1, dontSendNotification);
        kFreq->setValue (bb.freq.load(), dontSendNotification); kGain->setValue (bb.gain.load(), dontSendNotification);
        kQ->setValue (bb.q.load(), dontSendNotification); kDyn->setValue (bb.dyn.load(), dontSendNotification);
        bandOn.selected = bb.on.load(); bandOn.repaint();
        repaintDynBtn();
        eqView.repaint(); repaint (stripR);
        if (bb.on.load()) proc.mixLab.eqOn = true;
        refresh();
    }
    void repaintDynBtn() { dynBtn.selected = proc.mixLab.band[(size_t) eqView.sel].dyn.load() > 0.001f; dynBtn.repaint(); }
    void compStyle (int s)
    {
        auto& m = proc.mixLab;
        m.compStyle = s; m.compOn = true;
        switch (s)
        {
            case kk::csClean:   m.ratio = 3; m.attack = 5; m.release = 100; m.knee = 3; break;
            case kk::csPunch:   m.ratio = 4; m.attack = 18; m.release = 70; m.knee = 4; break;
            case kk::csGlue:    m.ratio = 2; m.attack = 30; m.release = 300; m.knee = 10; break;
            case kk::csOpto:    m.ratio = 3; m.attack = 10; m.release = 250; m.knee = 12; break;
            default:            m.ratio = 8; m.attack = 0.8f; m.release = 60; m.knee = 2; break;
        }
        syncKnobs(); refresh();
    }
    // ---- EQ EVOLVE
    void startEvolve()
    {
        if (variantsShown.empty()) evoOriginal = kk::coach::snap (proc.mixLab);
        else kk::coach::apply (proc.mixLab, evoOriginal);
        variantsShown.clear();
        const uint32 seed = (uint32) Time::getMillisecondCounter();
        for (int k = 0; k < 4; ++k) variantsShown.push_back (kk::coach::mutate (evoOriginal, seed + (uint32) k * 7919u, 0.25f + 0.2f * (float) k));
        proc.mixLab.eqOn = true;
        hoverVariant = -1;
        variantStrip.setVisible (true); variantStrip.toFront (false); variantStrip.repaint();
        evolveEqBtn.setButtonText ("4 NEW");
        note = "hover a curve = hear it, click = keep it (levels matched so louder does not win)";
        repaint();
    }
    void stopEvolve (bool keep)
    {
        if (variantsShown.empty()) return;
        if (! keep) kk::coach::apply (proc.mixLab, evoOriginal);
        variantsShown.clear(); hoverVariant = -1;
        variantStrip.setVisible (false);
        evolveEqBtn.setButtonText ("EVOLVE");
        selectBand (eqView.sel);
        repaint();
    }
    Rectangle<int> variantRect (int k) const
    {
        const int w = (variantStrip.getWidth() - 36) / 4;
        return { k * (w + 12), 4, w, 90 };
    }
    void paintVariants (Graphics& g)
    {
        for (int k = 0; k < (int) variantsShown.size(); ++k)
        {
            const auto r = variantRect (k).toFloat();
            g.setColour (Colours::black.withAlpha (k == hoverVariant ? 0.82f : 0.66f)); g.fillRoundedRectangle (r, 10);
            g.setColour (k == hoverVariant ? kk::theme().accent : Colours::white.withAlpha (0.3f)); g.drawRoundedRectangle (r, 10, k == hoverVariant ? 2.0f : 1.0f);
            const auto a = r.reduced (10, 16);
            Path p; bool first = true;
            for (float x = a.getX(); x <= a.getRight(); x += 2.0f)
            {
                const float hz = 20.0f * std::pow (1000.0f, (x - a.getX()) / a.getWidth());
                const float y = a.getCentreY() - a.getHeight() * 0.5f * jlimit (-1.0f, 1.0f, kk::coach::curveDb (variantsShown[(size_t) k], hz, an.sr) / 15.0f);
                if (first) { p.startNewSubPath (x, y); first = false; } else p.lineTo (x, y);
            }
            g.setColour (bandColour (k * 2 + 1)); g.strokePath (p, PathStrokeType (2.0f));
            g.setColour (Colours::white.withAlpha (0.75f)); g.setFont (kk::modern::font (11.0f, true, 0.15f));
            g.drawText (k == hoverVariant ? "CLICK = KEEP" : "CURVE " + String (k + 1), r.reduced (8, 3).removeFromTop (14), Justification::centredLeft);
        }
    }
    void hoverAt (Point<int> p)
    {
        if (variantsShown.empty()) return;
        int h = -1;
        for (int k = 0; k < (int) variantsShown.size(); ++k) if (variantRect (k).contains (p)) h = k;
        if (h != hoverVariant)
        {
            hoverVariant = h;
            kk::coach::apply (proc.mixLab, h >= 0 ? variantsShown[(size_t) h] : evoOriginal);
            eqView.repaint(); variantStrip.repaint();
        }
    }
    void pickAt (Point<int> p)
    {
        for (int k = 0; k < (int) variantsShown.size(); ++k)
            if (variantRect (k).contains (p))
            {
                kk::coach::apply (proc.mixLab, variantsShown[(size_t) k]);
                evoOriginal = variantsShown[(size_t) k];
                note = "kept curve " + String (k + 1) + " - EVOLVE again to grow from it";
                stopEvolve (true);
                return;
            }
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int b = 0; b < kk::MixLabState::numBands; ++b) if (view == 0 && chip (b).contains (e.getPosition())) { selectBand (b); return; }
    }

    // ---- COACH
    void refreshReading()
    {
        reading = kk::coach::read (an.longPow, an.sr, proc.mixLab, &an.persist);
        tips = kk::coach::advise (reading, proc.coachGenre, wholeBeat());
        if ((int) tips.size() > maxTips()) tips.resize ((size_t) maxTips());
        if (! reading.silent) scoreSm = scoreSm < 0 ? (float) kk::coach::score (tips) : scoreSm * 0.8f + 0.2f * (float) kk::coach::score (tips);
        layoutFixButtons();
    }
    void layoutFixButtons()
    {
        for (int i = 0; i < (int) fixBtns.size(); ++i)
        {
            const bool show = i < (int) tips.size() && tips[(size_t) i].fix != kk::coach::fixNone;
            fixBtns[(size_t) i]->setVisible (show);
            if (show)
            {
                const auto r = tipRect (i);
                fixBtns[(size_t) i]->setButtonText (tips[(size_t) i].fixLabel);
                fixBtns[(size_t) i]->setBounds (r.getRight() - 118, r.getY() + 6, 112, 26);
            }
        }
    }
    void applyTip (int i)
    {
        if (i >= (int) tips.size()) return;
        const auto& t = tips[(size_t) i];
        if (t.fix == kk::coach::fixUseKey) { proc.melKey = t.region / 2; proc.melScale = (t.region % 2) ? kk::mel::scMinor : kk::mel::scMajor; ++proc.melVer; note = "the MELODY page now writes in " + t.title.fromFirstOccurrenceOf ("IN ", false, false); }
        else note = "FIX: " + kk::coach::applyFix (proc.mixLab, t);
        ++fixes;
        syncKnobs(); refresh();
    }
    void paintCoach (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto C = coachArea();
        kk::modern::plate (g, C.toFloat(), 16.0f, true);
        g.setColour (t.text); g.setFont (kk::modern::font (20.0f, true, 0.2f));
        g.drawText ("COACH", C.getX() + 16, C.getY() + 14, 120, 28, Justification::centredLeft);
        // score ring
        const auto ring = Rectangle<float> ((float) C.getX() + 18, (float) C.getY() + 54, 96, 96);
        g.setColour (t.text.withAlpha (0.12f)); g.drawEllipse (ring.reduced (5), 8.0f);
        const bool live = ! reading.silent && scoreSm >= 0;
        const float sc = live ? scoreSm : 0.0f;
        if (live)
        {
            Path arc; arc.addCentredArc (ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f - 5, ring.getHeight() * 0.5f - 5, 0, 0, MathConstants<float>::twoPi * sc / 100.0f, true);
            const Colour c = sc >= 80 ? Colour (0xff36ff6a) : sc >= 55 ? Colour (0xffffd23f) : Colour (0xffff5a6d);
            g.setColour (c); g.strokePath (arc, PathStrokeType (8.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.0f));
        g.drawText (live ? String (roundToInt (sc)) : String ("--"), ring.toNearestInt(), Justification::centred);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (17.0f, true, 0.2f));
        g.drawText (live ? kk::coach::level (roundToInt (sc)) : "LISTENING", C.getX() + 128, C.getY() + 62, 220, 24, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.03f));
        g.drawFittedText (live ? String (wholeBeat() ? "BEAT SCORE - " : "SOUND SCORE - ") + (proc.coachGenre >= 0 ? String ("compared with ") + kk::mel::genreName (proc.coachGenre) : String ("balanced reference")) + "\n" + String (fixes) + " fixes applied"
                               : String ("play something: a melody, the keys,\nor your beat through EVOLVE on the master"),
                          Rectangle<int> (C.getX() + 128, C.getY() + 88, C.getWidth() - 140, 50), Justification::topLeft, 3, 0.8f);
        // tonal balance: your sound (bars) vs the style (ticks)
        const auto bal = Rectangle<float> ((float) C.getX() + 16, (float) C.getY() + 162, (float) C.getWidth() - 32, 112);
        g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (bal, 8);
        const auto T = kk::coach::target (proc.coachGenre, wholeBeat());
        const float bw = bal.getWidth() / (float) kk::coach::numRegions;
        auto yOf = [&] (float db) { return bal.getY() + 14 + (bal.getHeight() - 34) * (0.5f - jlimit (-0.5f, 0.5f, db / 30.0f)); };
        for (int i = 0; i < kk::coach::numRegions; ++i)
        {
            const float x = bal.getX() + bw * (float) i;
            if (live)
            {
                const float v = reading.region[(size_t) i], dv = v - T[(size_t) i];
                const Colour c = std::abs (dv) < 3.5f ? Colour (0xff36ff6a) : std::abs (dv) < 6 ? Colour (0xffffd23f) : Colour (0xffff5a6d);
                const float y0 = yOf (0), y1 = yOf (v);
                g.setColour (c.withAlpha (0.75f)); g.fillRoundedRectangle (x + bw * 0.22f, jmin (y0, y1), bw * 0.56f, jmax (2.0f, std::abs (y1 - y0)), 3);
            }
            g.setColour (t.text); g.fillRect (x + bw * 0.12f, yOf (T[(size_t) i]) - 1, bw * 0.76f, 2.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (9.5f, true, 0.0f));
            g.drawFittedText (kk::coach::regionName (i), Rectangle<int> ((int) x, (int) bal.getBottom() - 18, (int) bw, 16), Justification::centred, 1, 0.6f);
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.1f));
        g.drawText ("TONAL BALANCE   bars = yours   lines = the style", bal.toNearestInt().translated (0, 2).withHeight (12), Justification::centred);
        // tips
        for (int i = 0; i < maxTips(); ++i)
        {
            const auto r = tipRect (i).toFloat();
            if (i >= (int) tips.size()) { if (i == 0 && live) { g.setColour (Colour (0xff36ff6a)); g.setFont (kk::modern::font (15.0f, true, 0.1f)); g.drawText ("NOTHING TO FIX - IT SOUNDS RIGHT", r.toNearestInt(), Justification::centred); } continue; }
            const auto& tp = tips[(size_t) i];
            const Colour sev = tp.severity >= 3 ? Colour (0xffff5a6d) : tp.severity == 2 ? Colour (0xffffd23f) : tp.severity == 1 ? Colour (0xff22d3ee) : Colour (0xffa78bfa);
            g.setColour (Colours::black.withAlpha (0.22f)); g.fillRoundedRectangle (r, 9);
            g.setColour (sev); g.fillRoundedRectangle (r.getX(), r.getY() + 6, 4, r.getHeight() - 12, 2);
            g.setColour (t.text); g.setFont (kk::modern::font (13.0f, true, 0.06f));
            g.drawFittedText (tp.title, Rectangle<int> ((int) r.getX() + 14, (int) r.getY() + 6, (int) r.getWidth() - (tp.fix != kk::coach::fixNone ? 140 : 24), 26), Justification::centredLeft, 1, 0.7f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, false, 0.0f));
            g.drawFittedText (tp.why, Rectangle<int> ((int) r.getX() + 14, (int) r.getY() + 34, (int) r.getWidth() - 24, (int) r.getHeight() - 38), Justification::topLeft, 4, 0.85f);
        }
        // lesson
        const auto les = Rectangle<int> (C.getX() + 16, C.getBottom() - 70, C.getWidth() - 32, 58);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (11.0f, true, 0.25f));
        g.drawText ("PRO TIP", les.withHeight (14), Justification::centredLeft);
        g.setColour (t.text); g.setFont (kk::modern::font (12.0f, false, 0.0f));
        const auto& L = kk::coach::lessons();
        g.drawFittedText (L[(int) ((Time::getMillisecondCounter() / 20000u) % (uint32) L.size())], les.withTrimmedTop (16), Justification::topLeft, 3, 0.85f);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        an.update (proc.mixLab, (float) proc.getSampleRate());
        if (view == 0) eqView.repaint();
        if (view == 1) compView.push();
        if (view == 2) for (auto& c : cards) c->tick();
        if (view == 3) spaceView.tick();
        if (++frame % 8 == 0) { refreshReading(); repaint (coachArea()); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    MixAnalyzer an;
    EqDisplay eqView; CompDisplay compView; EraTimeline era; SpaceView spaceView; PaintOverlay variantStrip;
    std::vector<std::unique_ptr<TmCard>> cards;
    HotButton tabSp { lnf };
    HotButton tabEq { lnf }, tabComp { lnf }, tabTm { lnf }, powEq { lnf }, powComp { lnf }, powTm { lnf };
    HotButton bandOn { lnf }, dynBtn { lnf }, autoEqBtn { lnf }, evolveEqBtn { lnf }, resetEqBtn { lnf };
    HotButton autoGainBtn { lnf }, scBtn { lnf }, autoCompBtn { lnf }, diceTm { lnf }, modeSound { lnf }, modeBeat { lnf };
    std::vector<std::unique_ptr<HotButton>> styleBtns, fixBtns;
    ComboBox typeBox, styleBox;
    std::unique_ptr<RackKnob> kThresh, kRatio, kAttack, kRelease, kKnee, kMix, kCompOut, kFreq, kGain, kQ, kDyn, kEqOut, kMag, kTmMix;
    std::array<std::atomic<float>, 4> dummy {};
    kk::coach::Reading reading; std::vector<kk::coach::Tip> tips;
    std::vector<kk::coach::EqSnap> variantsShown; kk::coach::EqSnap evoOriginal; int hoverVariant = -1;
    Rectangle<int> stripR;
    String note;
    int view = 0, frame = 0, fixes = 0, coachMode = -1; float scoreSm = -1;   // coachMode -1 = AUTO (audio coming in = WHOLE BEAT)
};
