// v0.44 MATTER STRIP (included by PluginEditor.cpp): the eight sound macros as one living landscape instead of eight knobs.
// Paint over it: the height under your finger is how much of that character the sound gets, and one stroke shapes
// several neighbours at once (like clay). Right-drag = smooth it out.  Double-click = back to how the sound was made.
class MatterStrip : public Component, public SettableTooltipClient, private Timer
{
public:
    MatterStrip (KeysKillaProcessor& p, std::vector<const char*> ids) : proc (p)
    {
        for (auto* id : ids) prms.push_back (proc.apvts.getParameter (id));
        names = { "DARK", "SPACE", "MOVEMENT", "WIDTH", "TEXTURE", "PUNCH", "DIRT", "MIX" };
        setTooltip ("Paint the sound: the higher the landscape under a word, the more of it.  One stroke shapes its neighbours too.  Right-drag = smooth.  Double-click = as made.");
        readParams();
        startTimerHz (30);
    }
    void setNames (const StringArray& n) { if (n.size() == 8 && n != names) { names = n; repaint(); } }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto r = getLocalBounds().toFloat().reduced (2);
        g.setGradientFill (ColourGradient (t.night ? Colour (0xff0d0e16) : Colour (0xffe4e1ea), 0, r.getY(), t.night ? Colour (0xff07080d) : Colour (0xffd4d0dc), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 14);   // opaque: the old knob sockets of the background never show through
        g.setColour (t.text.withAlpha (0.12f)); g.drawRoundedRectangle (r, 14, 1.0f);
        const auto a = area();
        // the landscape: a smooth curve through the eight heights
        Path land, edge;
        const int n = 160;
        for (int i = 0; i <= n; ++i)
        {
            const float u = (float) i / (float) n, v = heightAt (u);
            const float wob = 0.012f * std::sin (u * 40.0f + phase * 2.0f) * v;
            const Point<float> pt (a.getX() + u * a.getWidth(), a.getBottom() - (v + wob) * a.getHeight());
            if (i == 0) { land.startNewSubPath (a.getX(), a.getBottom()); land.lineTo (pt); edge.startNewSubPath (pt); }
            else { land.lineTo (pt); edge.lineTo (pt); }
        }
        land.lineTo (a.getRight(), a.getBottom()); land.closeSubPath();
        const auto ac = t.accent;
        g.setGradientFill (ColourGradient (ac.withAlpha (0.55f), 0, a.getY(), Colour (0xff8f7cff).withAlpha (0.12f), 0, a.getBottom(), false));
        g.fillPath (land);
        g.setColour (ac.withAlpha (0.25f)); g.strokePath (edge, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour ((t.night ? Colours::white : Colour (0xff20202a)).withAlpha (0.85f)); g.strokePath (edge, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
        // sparks rising from the high places
        for (int k = 0; k < 24; ++k)
        {
            const float u = std::fmod ((float) k * 0.137f + 0.03f * phase * (0.5f + (float) (k % 3)), 1.0f), v = heightAt (u);
            const float life = std::fmod (phase * 0.3f + (float) k * 0.41f, 1.0f);
            g.setColour (Colours::white.withAlpha (0.5f * v * (1.0f - life))); g.fillEllipse (a.getX() + u * a.getWidth() - 1.5f, a.getBottom() - v * a.getHeight() - life * 18.0f - 1.5f, 3, 3);
        }
        // the words: brighter where there is more of them
        for (int i = 0; i < 8; ++i)
        {
            const float u = ((float) i + 0.5f) / 8.0f, v = vals[(size_t) i];
            g.setColour (t.text.withAlpha (0.35f + 0.6f * v)); g.setFont (kk::modern::font (11.5f + 2.0f * v, true, 0.22f));
            g.drawText (names[i], Rectangle<float> (a.getX() + u * a.getWidth() - 70, r.getBottom() - 22, 140, 18), Justification::centred);
        }
        if (brush.x > 0) { g.setColour (Colours::white.withAlpha (0.6f)); g.drawEllipse (brush.x - 16, brush.y - 16, 32, 32, 1.5f); }
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (auto* p : prms) p->beginChangeGesture();
        painting = true; smooth = e.mods.isPopupMenu();
        stroke (e.position);
    }
    void mouseDrag (const MouseEvent& e) override { stroke (e.position); }
    void mouseUp (const MouseEvent&) override { for (auto* p : prms) p->endChangeGesture(); painting = false; brush = { -1, -1 }; repaint(); }
    void mouseDoubleClick (const MouseEvent&) override
    {
        for (size_t i = 0; i < prms.size(); ++i) { prms[i]->beginChangeGesture(); prms[i]->setValueNotifyingHost (prms[i]->getDefaultValue()); prms[i]->endChangeGesture(); }
        readParams(); repaint();
    }
private:
    Rectangle<float> area() const { return getLocalBounds().toFloat().reduced (18, 10).withTrimmedBottom (18); }
    float heightAt (float u) const
    {
        // smooth interpolation through the zone centres
        const float x = u * 8.0f - 0.5f;
        const int i0 = (int) std::floor (x);
        const float f = x - (float) i0, s = f * f * (3.0f - 2.0f * f);
        const float a = vals[(size_t) jlimit (0, 7, i0)], b = vals[(size_t) jlimit (0, 7, i0 + 1)];
        return jlimit (0.0f, 1.0f, a + (b - a) * s);
    }
    void stroke (Point<float> p)
    {
        const auto a = area();
        brush = p;
        const float u = (p.x - a.getX()) / a.getWidth(), v = jlimit (0.0f, 1.0f, (a.getBottom() - p.y) / a.getHeight());
        for (int i = 0; i < 8; ++i)
        {
            const float zu = ((float) i + 0.5f) / 8.0f, w = std::exp (-std::pow ((zu - u) / 0.085f, 2.0f));
            if (w < 0.02f) continue;
            float& x = vals[(size_t) i];
            if (smooth) { const float nb = 0.5f * (vals[(size_t) jmax (0, i - 1)] + vals[(size_t) jmin (7, i + 1)]); x += (nb - x) * 0.25f * w; }
            else x += (v - x) * 0.6f * w;
            if (i < (int) prms.size()) prms[(size_t) i]->setValueNotifyingHost (x);
        }
        repaint();
    }
    void readParams() { for (size_t i = 0; i < prms.size() && i < 8; ++i) vals[i] = prms[i]->getValue(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        if (! painting)
        {
            bool changed = false;
            for (size_t i = 0; i < prms.size() && i < 8; ++i) if (std::abs (prms[i]->getValue() - vals[i]) > 1.0e-4f) { vals[i] = prms[i]->getValue(); changed = true; }
            juce::ignoreUnused (changed);
        }
        repaint();
    }
    KeysKillaProcessor& proc;
    std::vector<RangedAudioParameter*> prms;
    std::array<float, 8> vals {};
    StringArray names;
    bool painting = false, smooth = false;
    Point<float> brush { -1, -1 };
    float phase = 0;
};
