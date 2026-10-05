// v0.45 WORLDS pages (included by PluginEditor.cpp after FeedPage.h): BIOSPHERE, GARAGE (FLEX) and PARTY - three playful sound worlds.
// Every world deals a ROLL of 10 sounds (cards: click = it is on your keys, KEEP / DRAG WAV / ROLL AGAIN). The sounds are ALCHEMY
// recipes (Worlds.h): nothing is rendered until you hear, drag or keep one. Timers only animate while the page is showing.
#include "Worlds.h"

namespace kk::worldsui
{
using namespace juce;
inline AffineTransform unitTo (Rectangle<float> r) { return AffineTransform::scale (r.getWidth(), r.getHeight()).translated (r.getX(), r.getY()); }
inline Path placed (Path p, const AffineTransform& t) { p.applyTransform (t); return p; }
inline Colour excCol (int e) { return Colour (kk::alc::exciters()[(size_t) jlimit (0, 4, e)].colour); }
inline Colour bodyCol (int b) { return Colour (kk::alc::bodies()[(size_t) jlimit (0, 4, b)].colour); }
// a thick line as a polygon in unit space (a = the box aspect, so the width stays even)
inline void thick (Path& p, float x1, float y1, float x2, float y2, float w, float a)
{
    const float dx = (x2 - x1) * a, dy = y2 - y1, l = std::max (1.0e-4f, std::sqrt (dx * dx + dy * dy));
    const float nx = -dy / l * w / a, ny = dx / l * w;
    p.startNewSubPath (x1 + nx, y1 + ny); p.lineTo (x2 + nx * 0.6f, y2 + ny * 0.6f); p.lineTo (x2 - nx * 0.6f, y2 - ny * 0.6f); p.lineTo (x1 - nx, y1 - ny); p.closeSubPath();
}
inline void glowPath (Graphics& g, const Path& p, Colour c, float amount)
{
    for (int k = 3; k >= 1; --k) { g.setColour (c.withAlpha (amount * 0.09f)); g.strokePath (p, PathStrokeType ((float) k * 5.0f, PathStrokeType::curved, PathStrokeType::rounded)); }
}
// a recipe drawn as a little sound shape: slow attack when frozen, a long tail when stretched, fast wiggles when bright,
// jagged when hot, a second trace when torn in two
inline void soundShape (Graphics& g, const kk::worlds::Recipe& r, Rectangle<float> a, float phase, bool on)
{
    const auto ec = excCol (r.exc), bc = bodyCol (r.body);
    auto trace = [&] (float detune, Colour c, float alpha)
    {
        Path p; const int n = 90;
        const float att = 0.03f + 0.35f * r.cool, len = 0.35f + 0.32f * (r.stretch + 1.0f), cyc = 5.0f + 9.0f * (r.bright + 1.0f) * (1.0f + detune);
        Random rr ((int64) r.seed);
        for (int i = 0; i <= n; ++i)
        {
            const float x = (float) i / (float) n;
            const float env = x < att ? x / att : std::exp (-(x - att) / std::max (0.05f, len) * 2.2f);
            const float jag = r.heat * 0.35f * (rr.nextFloat() - 0.5f);
            const float y = env * (std::sin ((x * cyc + (on ? phase * 0.6f : 0.0f)) * MathConstants<float>::twoPi) * (0.8f - 0.3f * r.matter) + jag);
            const Point<float> pt (a.getX() + x * a.getWidth(), a.getCentreY() - y * a.getHeight() * 0.45f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (c.withAlpha (alpha)); g.strokePath (p, PathStrokeType (on ? 2.0f : 1.5f, PathStrokeType::curved, PathStrokeType::rounded));
    };
    if (r.split > 0.05f) trace (0.07f * r.split, bc, 0.35f + 0.4f * r.split);
    trace (0.0f, ec.brighter (0.3f), on ? 1.0f : 0.8f);
}
} // namespace kk::worldsui

//==============================================================================
// the common frame of a world: header, the stage (the world), a side panel, the ROLL strip of 10 cards and the buttons
class WorldPage : public Component, protected Timer
{
public:
    WorldPage (KeysKillaProcessor& p, KKLookAndFeel& l, Colour accent) : proc (p), lnf (l), acc (accent)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (playBtn, "PLAY", "Hear the chosen sound again - it is on your keys", [this] { hear (true); });
        btn (rollBtn, "ROLL AGAIN", "Ten new sounds from the same choice", [this] { seed = (uint32) Random::getSystemRandom().nextInt() | 1u; reroll (true); });
        btn (keepBtn, "KEEP", "Keep the chosen sound in your folders / sound kits", [this] { keep(); });
        rollBtn.hero = true;
        dragWav.makeFile = [this] { auto* g = selGenome(); return g != nullptr ? proc.exportGenomeWav (*g) : File(); };
        dragWav.setTooltip ("Drag the chosen sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        startTimerHz (30);
    }
    ~WorldPage() override { stopTimer(); }
    void visibilityChanged() override {}   // v0.45.2: opening a page never changes the sound on the keys
    void resized() override
    {
        const auto pn = panel(); const int bw = (pn.getWidth() - 30) / 2, x = pn.getX() + 10;
        playBtn.setBounds (x, pn.getBottom() - 104, bw, 42); rollBtn.setBounds (x + bw + 10, pn.getBottom() - 104, bw, 42);
        keepBtn.setBounds (x, pn.getBottom() - 54, bw, 42); dragWav.setBounds (x + bw + 10, pn.getBottom() - 55, bw, 44);
        layoutTabs();
    }
    // the sound on the keys (tests / the lead)
    const kk::worlds::Recipe* chosen() const { return isPositiveAndBelow (sel, (int) roll.size()) ? &roll[(size_t) sel] : nullptr; }
    const std::vector<kk::worlds::Recipe>& currentRoll() const { return roll; }
    void pickCard (int i, bool audition) { if (isPositiveAndBelow (i, (int) roll.size())) { sel = i; hear (audition); repaint(); } }
protected:
    virtual void reroll (bool audition) = 0;
    virtual void layoutTabs() {}
    virtual void heard() {}
    static constexpr int panelW = 340;
    Rectangle<int> stage() const { return { 24, 64, getWidth() - 24 - 16 - panelW - 24, getHeight() - 64 - 188 }; }
    Rectangle<int> panel() const { return { getWidth() - 24 - panelW, 64, panelW, getHeight() - 64 - 188 }; }
    Rectangle<int> panelTop() const { return panel().reduced (16, 14).withTrimmedBottom (showDna ? 300 : 196); }
    Rectangle<int> strip() const { return { 24, getHeight() - 176, getWidth() - 48, 164 }; }
    Rectangle<float> card (int i) const { const auto s = strip().toFloat(); const float w = (s.getWidth() - 24 - 9 * 10) / 10.0f; return { s.getX() + 12 + (float) i * (w + 10), s.getY() + 34, w, s.getHeight() - 44 }; }
    void layoutTabRow (std::initializer_list<HotButton*> tabs, int w)
    {
        int x = getWidth() - 24 - (int) tabs.size() * (w + 8) + 8;
        for (auto* b : tabs) { b->setBounds (x, 14, w, 36); x += w + 8; }
    }
    void setRoll (std::vector<kk::worlds::Recipe> r, bool audition, int newSel = 0)
    {
        roll = std::move (r); gens.assign (roll.size(), {});
        sel = roll.empty() ? -1 : jlimit (0, (int) roll.size() - 1, newSel);
        if (audition || isShowing()) hear (audition);   // built while hidden: the keys change only when you come to the page
        repaint();
    }
    void replaceSel (const kk::worlds::Recipe& r, bool audition)
    {
        if (! isPositiveAndBelow (sel, (int) roll.size())) return;
        roll[(size_t) sel] = r; gens[(size_t) sel] = {}; hear (audition); repaint();
    }
    const KeysKillaProcessor::Genome* selGenome()
    {
        if (! isPositiveAndBelow (sel, (int) roll.size())) return nullptr;
        auto& g = gens[(size_t) sel];
        if (! g.valid()) g = kk::worlds::genome (proc, roll[(size_t) sel]);   // made only when it is heard / dragged / kept
        return g.valid() ? &g : nullptr;
    }
    void hear (bool audition) { if (auto* g = selGenome()) { proc.alcUse (*g, audition); cardFlash = 1.0f; heard(); } }
    void keep()
    {
        auto* g = selGenome(); if (g == nullptr) return;
        const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
        auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (*g, rate, 3.0), rate, rate, g->name);
        saveToFolderMenu (proc, { snd }, &keepBtn, [safe = SafePointer<WorldPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
    }
    bool stripDown (const MouseEvent& e)
    {
        for (int i = 0; i < (int) roll.size(); ++i) if (card (i).contains (e.position)) { sel = i; hear (true); repaint(); return true; }
        return false;
    }
    void stripHover (Point<float> p) { int h = -1; for (int i = 0; i < (int) roll.size(); ++i) if (card (i).contains (p)) h = i; if (h != hoverCard) { hoverCard = h; repaint (strip()); } }

    void drawHeader (Graphics& g, const String& title, const String& hint)
    {
        const auto& t = kk::theme();
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        const int tw = (int) std::ceil (GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 22;
        g.drawText (title, 24, 12, tw, 40, Justification::centredLeft);
        g.setColour (acc); g.fillRoundedRectangle (24.0f, 50.0f, (float) tw - 22.0f, 3.0f, 1.5f);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, true, 0.04f));
        g.drawFittedText (hint, 24 + tw, 12, getWidth() - 24 - tabsW - 20 - (24 + tw), 40, Justification::centredLeft, 2, 0.85f);
    }
    void drawPanel (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto pn = panel().toFloat();
        kk::modern::well (g, pn, 16.0f);
        const float y = pn.getBottom() - 192;
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.3f));
        g.drawText ("ON YOUR KEYS", Rectangle<float> (pn.getX() + 16, y, pn.getWidth() - 32, 16), Justification::centredLeft);
        g.setColour (acc.withAlpha (0.5f)); g.fillRect (pn.getX() + 16, y - 6, pn.getWidth() - 32, 1.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (21.0f, true, 0.03f));
        g.drawFittedText (chosen() != nullptr ? chosen()->name : String ("-"), Rectangle<float> (pn.getX() + 16, y + 16, pn.getWidth() - 32, 52).toNearestInt(), Justification::centredLeft, 2, 0.75f);
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.0f, true, 0.03f)); g.drawFittedText (note, Rectangle<float> (pn.getX() + 16, y + 68, pn.getWidth() - 32, 18).toNearestInt(), Justification::centredLeft, 1, 0.8f); }
        if (showDna && chosen() != nullptr) drawDna (g, Rectangle<float> (pn.getX() + 16, y - 104, pn.getWidth() - 32, 90), *chosen());
    }
    // what the chosen recipe is made of - glowing meters, no numbers
    void drawDna (Graphics& g, Rectangle<float> a, const kk::worlds::Recipe& r)
    {
        const auto& t = kk::theme();
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.3f));
        g.drawText ("THE SOUND", a.withHeight (14), Justification::centredLeft);
        g.setColour (t.night ? acc.withAlpha (0.85f) : acc.darker (1.2f)); g.drawText ((String (kk::alc::exciters()[(size_t) r.exc].word) + "  x  " + kk::alc::bodies()[(size_t) r.body].word).toUpperCase(), a.withHeight (14).withTrimmedLeft (100), Justification::centredRight);
        const char* names[] { "GLASS", "LONG", "BRIGHT", "HOT", "FROZEN", "LAYERS" };
        const float v[] { 1.0f - r.matter, (r.stretch + 1.0f) * 0.5f, (r.bright + 1.0f) * 0.5f, r.heat, r.cool, r.split };
        const float cw = a.getWidth() / 6.0f;
        for (int i = 0; i < 6; ++i)
        {
            const auto col = Rectangle<float> (a.getX() + (float) i * cw, a.getY() + 20, cw, a.getHeight() - 20);
            const auto bar = Rectangle<float> (col.getCentreX() - 7, col.getY(), 14, col.getHeight() - 18);
            g.setColour (acc.withAlpha (0.12f)); g.fillRoundedRectangle (bar, 4);
            const float h = jmax (3.0f, bar.getHeight() * jlimit (0.0f, 1.0f, v[i]));
            const auto fill = bar.withTrimmedTop (bar.getHeight() - h);
            g.setGradientFill (ColourGradient (acc.brighter (0.4f), fill.getX(), fill.getY(), acc.darker (0.5f), fill.getX(), fill.getBottom(), false)); g.fillRoundedRectangle (fill, 4);
            g.setColour (t.dim); g.setFont (kk::modern::font (9.0f, true, 0.12f));
            g.drawText (names[i], col.withTrimmedTop (col.getHeight() - 14), Justification::centred);
        }
    }
    void drawStrip (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto s = strip().toFloat();
        kk::modern::well (g, s, 16.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (13.5f, true, 0.28f));
        g.drawText ("YOUR ROLL", Rectangle<float> (s.getX() + 16, s.getY() + 8, 140, 20), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawText ("ten sounds - click one = it is on your keys.   KEEP = into your folders,   DRAG WAV = into FL,   ROLL AGAIN = ten new ones", Rectangle<float> (s.getX() + 170, s.getY() + 10, s.getWidth() - 190, 18), Justification::centredLeft);
        for (int i = 0; i < (int) roll.size() && i < 10; ++i) drawCard (g, i);
    }
    void drawCard (Graphics& g, int i)
    {
        const auto r = card (i); const auto& rc = roll[(size_t) i];
        const bool on = i == sel, hov = i == hoverCard;
        const auto ec = kk::worldsui::excCol (rc.exc), bc = kk::worldsui::bodyCol (rc.body);
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 4), 12);
        g.setGradientFill (ColourGradient (Colour (0xff17141f), r.getX(), r.getY(), Colour (0xff07080d), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 12);
        g.setGradientFill (ColourGradient (ec.withAlpha (on ? 0.42f : hov ? 0.3f : 0.18f), r.getX(), r.getY(), bc.withAlpha (on ? 0.36f : 0.14f), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 12);
        kk::worldsui::soundShape (g, rc, Rectangle<float> (r.getX() + 10, r.getY() + 12, r.getWidth() - 20, r.getHeight() * 0.38f), phase, on);
        g.setColour (Colours::white.withAlpha (on ? 1.0f : 0.85f)); g.setFont (kk::modern::font (12.5f, true, 0.03f));
        g.drawFittedText (rc.name, r.reduced (8, 0).withTrimmedTop (r.getHeight() * 0.52f).withTrimmedBottom (6).toNearestInt(), Justification::centred, 3, 0.8f);
        if (on)
        {
            Path b; b.addRoundedRectangle (r.reduced (0.5f), 12);
            kk::worldsui::glowPath (g, b, acc, 0.6f + 0.6f * cardFlash);
            g.setColour (acc); g.strokePath (b, PathStrokeType (2.4f));
        }
        else { g.setColour (Colours::white.withAlpha (hov ? 0.4f : 0.16f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, 1.0f); }
    }
    void tickBase() { phase += 0.05f; if (cardFlash > 0) cardFlash = jmax (0.0f, cardFlash - 0.06f); }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    Colour acc;
    HotButton playBtn { lnf }, rollBtn { lnf }, keepBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    std::vector<kk::worlds::Recipe> roll;
    std::vector<KeysKillaProcessor::Genome> gens;
    int sel = -1, hoverCard = -1, tabsW = 0;
    bool showDna = true;
    uint32 seed = 1;
    float phase = 0, cardFlash = 0;
    String note;
};

//==============================================================================
// BIOSPHERE: ANIMALS / NATURE / HUMAN BODY. Click a part = it is coloured in and makes its sound. The animals live: move fast
// near one = it PANICS (shakes, runs; its sounds go wild), come slowly = it is SHY (it hides; softer, darker), stay with it a while
// (or double-click it) = it MUTATES (new colours, extra eyes and limbs; a new mutation of the sound, torn in two).
class BiospherePage : public WorldPage
{
public:
    BiospherePage (KeysKillaProcessor& p, KKLookAndFeel& l) : WorldPage (p, l, Colour (0xff36ff9a))
    {
        for (int k = 0; k < kk::worlds::numKingdoms; ++k)
        {
            auto& b = *tabs[(size_t) k];
            b.setButtonText (kk::worlds::kingdomName (k)); b.framed = true; b.onClick = [this, k] { setKingdom (k); }; addAndMakeVisible (b);
        }
        tabs[0]->setTooltip ("Animals: colour a part, scare them, sneak up on them, let them mutate");
        tabs[1]->setTooltip ("Nature: a tree, a forest, the ocean, a storm, the desert");
        tabs[2]->setTooltip ("The human body like the anatomy model at school: brain, throat, lungs, heart, blood, bones");
        tabsW = 3 * 158;
        seed = 7;
        setKingdom (0, false);
        setMouseCursor (MouseCursor::PointingHandCursor);
    }
    // debug / snapshot: 80 = animals with a panicking bird, a shy cat and a mutant snake; 81 = the human body with lit parts
    void debugShow (int view)
    {
        if (view == 81)
        {
            setKingdom (2, false);
            lit[2][0] = 0b001111u; lastPart = 3; focus[2] = 0;
            reroll (false); phase = 2.0f;
            return;
        }
        setKingdom (0, false);
        lit[0][0] = 0b0011u; lit[0][2] = 0b0110u; lit[0][5] = 0b1001u; lit[0][1] = 0b0001u;
        moods[0][2].panic = 1.0f; moods[0][4].shy = 0.85f; moods[0][5].mutations = 2;
        focus[0] = 2; lastPart = 1;
        reroll (false); sel = 1; phase = 3.1f;
    }
    void visibilityChanged() override { mouseIn = false; WorldPage::visibilityChanged(); }
    void paint (Graphics& g) override
    {
        pageBackdrop (g, *this);
        drawHeader (g, "BIOSPHERE", "colour a part = it sings.  move fast near an animal = it PANICS,  come slowly = it is SHY,  stay with it (or double-click) = it MUTATES");
        drawStage (g);
        drawPanel (g);
        drawInfo (g);
        drawStrip (g);
    }
    void layoutTabs() override { layoutTabRow ({ tabs[0], tabs[1], tabs[2] }, 150); }

    void mouseMove (const MouseEvent& e) override
    {
        const auto now = Time::getMillisecondCounter();
        const float dt = jmax (1.0f, (float) (now - lastMs)) / 1000.0f;
        const float inst = e.position.getDistanceFrom (mpos) / dt;
        speed = dt > 0.25f ? inst * 0.2f : speed * 0.55f + inst * 0.45f;
        mpos = e.position; lastMs = now; mouseIn = stage().contains (e.getPosition());
        int hb = -1, hp = -1;
        if (mouseIn) for (int i = numBeings() - 1; i >= 0 && hb < 0; --i) { const auto u = toUnit (i, mpos); if (const int pp = kk::worlds::partAt (kingdom, i, u.x, u.y); pp >= 0) { hb = i; hp = pp; } }
        if (kingdom == kk::worlds::kBody && hb < 0) for (int i = 0; i < numParts (0); ++i) if (labelRect (i).contains (mpos)) { hb = 0; hp = i; }
        if (hb != hoverBeing || hp != hoverPart) { hoverBeing = hb; hoverPart = hp; }
        stripHover (e.position);
    }
    void mouseExit (const MouseEvent&) override { mouseIn = false; hoverBeing = hoverPart = -1; hoverCard = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        if (stripDown (e)) return;
        int bi = -1, pi = -1;
        for (int i = numBeings() - 1; i >= 0 && bi < 0; --i)
        {
            const auto u = toUnit (i, e.position);
            if (const int pp = kk::worlds::partAt (kingdom, i, u.x, u.y); pp >= 0) { bi = i; pi = pp; }
            else if (u.x > 0.0f && u.x < 1.0f && u.y > 0.0f && u.y < 1.0f) bi = i;
        }
        if (kingdom == kk::worlds::kBody && pi < 0) for (int i = 0; i < numParts (0); ++i) if (labelRect (i).contains (e.position)) { bi = 0; pi = i; }
        if (bi < 0) return;
        if (pi < 0) { focus[(size_t) kingdom] = bi; lastPart = -1; reroll (true); note = String (being (bi).name) + " - colour its parts"; return; }
        if (e.mods.isPopupMenu()) { lit[(size_t) kingdom][(size_t) bi] &= ~(1u << pi); focus[(size_t) kingdom] = bi; reroll (false); note = String (being (bi).parts[(size_t) pi].name) + " washed off"; return; }
        colourPart (bi, pi);
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        for (int i = numBeings() - 1; i >= 0; --i)
        {
            const auto u = toUnit (i, e.position);
            if (u.x > 0.0f && u.x < 1.0f && u.y > 0.0f && u.y < 1.0f) { mutate (i); return; }
        }
    }
    // a click on a part: it is coloured in and its sound plays (again = the next variation of that part)
    void colourPart (int bi, int pi)
    {
        auto& m = lit[(size_t) kingdom][(size_t) bi];
        const bool was = (m >> pi) & 1u;
        const bool sameFocus = focus[(size_t) kingdom] == bi && lastPart == pi;
        m |= 1u << pi; focus[(size_t) kingdom] = bi; lastPart = pi;
        flashBeing = bi; flashPart = pi; flash = 1.0f;
        int nlit = 0; for (int k = 0; k < 8; ++k) nlit += (m >> k) & 1;
        const int nextSel = was && sameFocus ? (sel + nlit) % 10 : 0;
        setRoll (kk::worlds::bioRoll (kingdom, bi, m, pi, moods[(size_t) kingdom][(size_t) bi], seed), true, nextSel);
        note = String (being (bi).parts[(size_t) pi].name) + " = " + being (bi).parts[(size_t) pi].trait;
    }
    void mutate (int bi)
    {
        auto& md = moods[(size_t) kingdom][(size_t) bi];
        md.mutations = jmin (6, md.mutations + 1); md.shy = 0;
        mutFlash[(size_t) bi] = 1.0f; focus[(size_t) kingdom] = bi;
        reroll (true);
        note = String (being (bi).name) + " MUTATED - a new sound, torn in two";
    }
private:
    int numBeings() const { return (int) kk::worlds::beings (kingdom).size(); }
    int numParts (int bi) const { return (int) being (bi).parts.size(); }
    const kk::worlds::Being& being (int bi) const { return kk::worlds::beings (kingdom)[(size_t) jlimit (0, numBeings() - 1, bi)]; }
    void setKingdom (int k, bool audition = true)
    {
        kingdom = jlimit (0, 2, k);
        for (int i = 0; i < 3; ++i) { tabs[(size_t) i]->selected = i == kingdom; tabs[(size_t) i]->repaint(); }
        lastPart = -1; hoverBeing = hoverPart = -1;
        reroll (audition);
    }
    void reroll (bool audition) override
    {
        const int f = focus[(size_t) kingdom];
        setRoll (kk::worlds::bioRoll (kingdom, f, lit[(size_t) kingdom][(size_t) f], lastPart, moods[(size_t) kingdom][(size_t) f], seed), audition);
    }
    Rectangle<float> beingBox (int i) const
    {
        const auto s = stage().toFloat().reduced (14, 10);
        if (kingdom == kk::worlds::kBody) { const float h = s.getHeight() - 10, w = h * 0.5f; return { s.getCentreX() - w * 0.5f, s.getY() + 6, w, h }; }
        const int perRow = 3; const int row = i / perRow, col = i % perRow;
        const int inRow = kingdom == kk::worlds::kNature && row == 1 ? 2 : 3;
        const float cw = s.getWidth() / 3.0f, ch = s.getHeight() / 2.0f;
        const float x0 = s.getX() + (s.getWidth() - cw * (float) inRow) * 0.5f;
        return Rectangle<float> (x0 + (float) col * cw, s.getY() + (float) row * ch, cw, ch).reduced (26, 14).withTrimmedBottom (24);
    }
    AffineTransform xform (int i) const
    {
        const auto b = beingBox (i); const auto& m = moods[(size_t) kingdom][(size_t) i];
        const float fi = (float) i;
        const float dx = m.panic * (std::sin (phase * 37.0f + fi) * 5.0f + std::sin (phase * 3.3f + fi * 2.0f) * b.getWidth() * 0.07f);
        const float dy = m.panic * std::cos (phase * 29.0f + fi) * 3.0f + m.shy * b.getHeight() * 0.12f;
        const float s = 1.0f - 0.24f * m.shy;
        const float rot = m.mutations > 0 ? 0.035f * std::sin (phase * 2.3f + fi) : m.panic * 0.05f * std::sin (phase * 31.0f + fi);
        return kk::worldsui::unitTo (b).scaled (s, s, b.getCentreX(), b.getBottom()).rotated (rot, b.getCentreX(), b.getCentreY()).translated (dx, dy);
    }
    Point<float> toUnit (int i, Point<float> p) const { float x = p.x, y = p.y; xform (i).inverted().transformPoint (x, y); return { x, y }; }
    Rectangle<float> labelRect (int pi) const
    {
        const auto b = beingBox (0); const auto& P = being (0).parts[(size_t) pi];
        const bool left = P.cx < 0.5f || (pi % 2 == 0 && P.cx == 0.5f);
        const float y = b.getY() + b.getHeight() * (0.06f + 0.155f * (float) pi);
        return left ? Rectangle<float> (b.getX() - 250, y, 170, 44) : Rectangle<float> (b.getRight() + 80, y, 170, 44);
    }
    // ---- the drawn beings (stylised vector art in the being's 0..1 box; A = its aspect, so circles stay round)
    Path bodyPath (int k, int i, float A) const
    {
        using kk::worldsui::thick;
        Path p;
        auto E = [&] (float cx, float cy, float rx, float ry) { p.addEllipse (cx - rx, cy - ry, rx * 2, ry * 2); };
        auto C = [&] (float cx, float cy, float r) { E (cx, cy, r / A, r); };
        if (k == kk::worlds::kAnimals)
        {
            switch (i)
            {
                case 0:   // wolf
                    E (0.45f, 0.52f, 0.27f, 0.17f); E (0.63f, 0.47f, 0.12f, 0.18f); C (0.79f, 0.31f, 0.13f);
                    p.addTriangle (0.82f, 0.24f, 0.98f, 0.36f, 0.82f, 0.42f);
                    p.addTriangle (0.73f, 0.24f, 0.75f, 0.05f, 0.80f, 0.20f); p.addTriangle (0.80f, 0.20f, 0.86f, 0.06f, 0.87f, 0.26f);
                    for (float x : { 0.25f, 0.34f, 0.58f, 0.67f }) p.addRoundedRectangle (x, 0.58f, 0.045f, 0.33f, 0.02f);
                    p.startNewSubPath (0.21f, 0.44f); p.quadraticTo (0.06f, 0.47f, 0.02f, 0.27f); p.quadraticTo (0.11f, 0.39f, 0.22f, 0.53f); p.closeSubPath();
                    break;
                case 1:   // whale
                    p.startNewSubPath (0.92f, 0.56f); p.cubicTo (0.92f, 0.30f, 0.55f, 0.30f, 0.30f, 0.42f); p.quadraticTo (0.18f, 0.50f, 0.12f, 0.56f);
                    p.quadraticTo (0.20f, 0.62f, 0.30f, 0.70f); p.cubicTo (0.55f, 0.84f, 0.92f, 0.80f, 0.92f, 0.56f); p.closeSubPath();
                    p.startNewSubPath (0.14f, 0.56f); p.quadraticTo (0.05f, 0.42f, 0.0f, 0.34f); p.quadraticTo (0.06f, 0.52f, 0.10f, 0.56f); p.quadraticTo (0.06f, 0.62f, 0.0f, 0.78f); p.quadraticTo (0.06f, 0.70f, 0.14f, 0.56f); p.closeSubPath();
                    p.addTriangle (0.44f, 0.68f, 0.57f, 0.70f, 0.41f, 0.88f);
                    break;
                case 2:   // bird
                    E (0.50f, 0.58f, 0.20f, 0.15f); C (0.72f, 0.38f, 0.12f);
                    p.addTriangle (0.78f, 0.32f, 0.96f, 0.37f, 0.78f, 0.43f);
                    p.startNewSubPath (0.36f, 0.52f); p.quadraticTo (0.30f, 0.18f, 0.20f, 0.10f); p.quadraticTo (0.52f, 0.16f, 0.62f, 0.50f); p.closeSubPath();
                    p.addTriangle (0.34f, 0.54f, 0.04f, 0.48f, 0.08f, 0.66f); p.addTriangle (0.34f, 0.60f, 0.08f, 0.66f, 0.14f, 0.78f);
                    thick (p, 0.48f, 0.70f, 0.46f, 0.92f, 0.012f, A); thick (p, 0.56f, 0.70f, 0.58f, 0.92f, 0.012f, A);
                    break;
                case 3:   // swarm: a hive and its bees
                {
                    const float r = 0.17f;
                    for (int v = 0; v < 6; ++v) { const float a = (float) v / 6.0f * MathConstants<float>::twoPi + 0.52f; const Point<float> q (0.52f + std::cos (a) * r / A, 0.50f + std::sin (a) * r); if (v == 0) p.startNewSubPath (q); else p.lineTo (q); }
                    p.closeSubPath();
                    Random rr (77);
                    for (int b = 0; b < 16; ++b)
                    {
                        const float bx = 0.08f + 0.84f * rr.nextFloat(), by = 0.1f + 0.8f * rr.nextFloat();
                        const float x = bx + 0.02f * std::sin (phase * (6.0f + (float) b) + (float) b), y = by + 0.03f * std::cos (phase * (5.0f + (float) b * 0.7f) + (float) b);
                        if (std::abs (x - 0.52f) < 0.15f && std::abs (y - 0.5f) < 0.2f) continue;
                        E (x, y, 0.035f / A * 1.3f, 0.032f);
                    }
                    break;
                }
                case 4:   // cat
                    E (0.45f, 0.62f, 0.18f, 0.27f); C (0.65f, 0.28f, 0.15f);
                    p.addTriangle (0.56f, 0.22f, 0.58f, 0.03f, 0.645f, 0.15f); p.addTriangle (0.655f, 0.15f, 0.73f, 0.03f, 0.745f, 0.22f);
                    p.startNewSubPath (0.30f, 0.84f); p.cubicTo (0.03f, 0.88f, 0.06f, 0.48f, 0.17f, 0.40f); p.quadraticTo (0.13f, 0.60f, 0.32f, 0.76f); p.closeSubPath();
                    E (0.40f, 0.89f, 0.065f, 0.045f); E (0.53f, 0.89f, 0.065f, 0.045f);
                    break;
                default:  // snake: a thick S with a taper
                {
                    std::vector<Point<float>> L, R;
                    const Point<float> P0 (0.06f, 0.78f), C1 (0.22f, 1.0f), C2 (0.36f, 0.42f), P1 (0.52f, 0.60f), C3 (0.66f, 0.78f), C4 (0.76f, 0.30f), P2 (0.84f, 0.30f);
                    auto bez = [] (Point<float> a, Point<float> b, Point<float> c, Point<float> d, float t) { const float u = 1 - t; return a * (u * u * u) + b * (3 * u * u * t) + c * (3 * u * t * t) + d * (t * t * t); };
                    const int n = 48;
                    for (int s = 0; s <= n; ++s)
                    {
                        const float t = (float) s / (float) n, u = t * 2.0f;
                        auto at = [&] (float uu) { return uu < 1.0f ? bez (P0, C1, C2, P1, uu) : bez (P1, C3, C4, P2, jmin (1.0f, uu - 1.0f)); };
                        auto pt = at (u); const auto pt2 = at (jmin (2.0f, u + 0.01f)), pt0 = at (jmax (0.0f, u - 0.01f));
                        pt.y += 0.018f * std::sin (phase * 3.0f + t * 9.0f);
                        const float dx = (pt2.x - pt0.x) * A, dy = pt2.y - pt0.y, l = std::max (1.0e-5f, std::sqrt (dx * dx + dy * dy));
                        const float w = 0.025f + 0.055f * std::sin (MathConstants<float>::pi * jlimit (0.0f, 1.0f, t * 1.1f));
                        L.push_back ({ pt.x - dy / l * w / A, pt.y + dx / l * w }); R.push_back ({ pt.x + dy / l * w / A, pt.y - dx / l * w });
                    }
                    p.startNewSubPath (L.front()); for (auto& q : L) p.lineTo (q); for (auto it = R.rbegin(); it != R.rend(); ++it) p.lineTo (*it); p.closeSubPath();
                    E (0.87f, 0.28f, 0.075f, 0.085f);
                    break;
                }
            }
        }
        else if (k == kk::worlds::kNature)
        {
            switch (i)
            {
                case 0:   // tree
                    p.startNewSubPath (0.44f, 0.94f); p.lineTo (0.47f, 0.45f); p.lineTo (0.53f, 0.45f); p.lineTo (0.56f, 0.94f); p.closeSubPath();
                    thick (p, 0.49f, 0.60f, 0.28f, 0.42f, 0.016f, A); thick (p, 0.51f, 0.55f, 0.72f, 0.40f, 0.016f, A);
                    E (0.33f, 0.30f, 0.17f, 0.15f); E (0.50f, 0.20f, 0.21f, 0.17f); E (0.67f, 0.30f, 0.17f, 0.15f); E (0.50f, 0.34f, 0.22f, 0.13f);
                    for (float x : { 0.26f, 0.38f, 0.62f, 0.74f }) thick (p, 0.5f, 0.91f, x, 0.985f, 0.012f, A);
                    break;
                case 1:   // forest
                    p.addRoundedRectangle (0.02f, 0.84f, 0.96f, 0.14f, 0.04f);
                    for (auto [x, h] : { std::pair<float, float> { 0.14f, 0.55f }, { 0.34f, 0.70f }, { 0.56f, 0.62f }, { 0.86f, 0.5f } })
                    {
                        p.addRectangle (x - 0.012f, 0.74f, 0.024f, 0.12f);
                        for (int t = 0; t < 3; ++t) { const float y0 = 0.76f - (float) t * h * 0.24f, ww = 0.085f - 0.018f * (float) t; p.addTriangle (x - ww, y0, x + ww, y0, x, y0 - h * 0.42f); }
                    }
                    p.startNewSubPath (0.70f, 0.72f); p.quadraticTo (0.78f, 0.58f, 0.86f, 0.72f); p.closeSubPath(); p.addRectangle (0.77f, 0.72f, 0.02f, 0.1f);
                    break;
                case 2:   // ocean
                {
                    p.startNewSubPath (0.0f, 0.98f); p.lineTo (0.0f, 0.32f);
                    for (int s = 1; s <= 40; ++s) { const float x = (float) s / 40.0f; p.lineTo (x, 0.32f + 0.035f * std::sin (x * 14.0f + phase * 2.0f)); }
                    p.lineTo (1.0f, 0.98f); p.closeSubPath();
                    break;
                }
                case 3:   // storm
                    E (0.30f, 0.20f, 0.17f, 0.11f); E (0.50f, 0.14f, 0.21f, 0.13f); E (0.70f, 0.21f, 0.17f, 0.11f); E (0.50f, 0.26f, 0.32f, 0.09f);
                    break;
                default:  // desert
                    p.startNewSubPath (0.0f, 0.98f); p.lineTo (0.0f, 0.80f); p.quadraticTo (0.25f, 0.66f, 0.52f, 0.82f); p.quadraticTo (0.76f, 0.70f, 1.0f, 0.84f); p.lineTo (1.0f, 0.98f); p.closeSubPath();
                    p.addRoundedRectangle (0.27f, 0.34f, 0.06f, 0.52f, 0.03f);
                    p.addRoundedRectangle (0.19f, 0.46f, 0.04f, 0.18f, 0.02f); p.addRectangle (0.20f, 0.60f, 0.08f, 0.04f);
                    p.addRoundedRectangle (0.36f, 0.40f, 0.04f, 0.16f, 0.02f); p.addRectangle (0.32f, 0.52f, 0.07f, 0.04f);
                    break;
            }
        }
        else
        {
            // the human figure (front view)
            C (0.5f, 0.075f, 0.062f);
            p.addRoundedRectangle (0.45f, 0.13f, 0.10f, 0.08f, 0.02f);
            Path torso; torso.startNewSubPath (0.22f, 0.21f); torso.lineTo (0.78f, 0.21f); torso.lineTo (0.70f, 0.50f); torso.lineTo (0.68f, 0.57f); torso.lineTo (0.32f, 0.57f); torso.lineTo (0.30f, 0.50f); torso.closeSubPath();
            p.addPath (torso.createPathWithRoundedCorners (0.03f));
            p.startNewSubPath (0.23f, 0.215f); p.lineTo (0.14f, 0.25f); p.lineTo (0.08f, 0.53f); p.lineTo (0.15f, 0.535f); p.lineTo (0.28f, 0.30f); p.closeSubPath();
            p.startNewSubPath (0.77f, 0.215f); p.lineTo (0.86f, 0.25f); p.lineTo (0.92f, 0.53f); p.lineTo (0.85f, 0.535f); p.lineTo (0.72f, 0.30f); p.closeSubPath();
            C (0.115f, 0.56f, 0.026f); C (0.885f, 0.56f, 0.026f);
            p.startNewSubPath (0.32f, 0.56f); p.lineTo (0.495f, 0.56f); p.lineTo (0.47f, 0.95f); p.lineTo (0.37f, 0.95f); p.closeSubPath();
            p.startNewSubPath (0.505f, 0.56f); p.lineTo (0.68f, 0.56f); p.lineTo (0.63f, 0.95f); p.lineTo (0.53f, 0.95f); p.closeSubPath();
            E (0.40f, 0.965f, 0.07f, 0.018f); E (0.60f, 0.965f, 0.07f, 0.018f);
        }
        return p;
    }
    Path partPath (const kk::worlds::BioPart& P, float pulse) const
    {
        using namespace kk::worlds;
        Path s; const float cx = P.cx, cy = P.cy, rx = P.rx * pulse, ry = P.ry * pulse;
        switch (P.shape)
        {
            case shWing: s.startNewSubPath (cx - rx, cy + ry * 0.6f); s.quadraticTo (cx - rx * 0.2f, cy - ry * 1.3f, cx + rx, cy - ry); s.quadraticTo (cx + rx * 0.3f, cy + ry * 0.1f, cx + rx * 0.4f, cy + ry * 0.9f); s.quadraticTo (cx - rx * 0.2f, cy + ry * 0.5f, cx - rx, cy + ry * 0.6f); s.closeSubPath(); break;
            case shSpikes: s.addRoundedRectangle (cx - rx, cy + ry * 0.2f, rx * 2, ry * 0.8f, ry * 0.3f); for (int k = 0; k < 4; ++k) { const float x0 = cx - rx + (float) k * rx * 0.5f; s.addTriangle (x0, cy + ry * 0.4f, x0 + rx * 0.5f, cy + ry * 0.4f, x0 + rx * 0.3f, cy - ry); } break;
            case shWave: { const int n = 24; s.startNewSubPath (cx - rx, cy - ry * 0.4f); for (int k = 1; k <= n; ++k) { const float x = (float) k / (float) n; s.lineTo (cx - rx + 2 * rx * x, cy - ry * 0.45f + ry * 0.35f * std::sin (x * 12.0f + phase * 3.0f)); }
                           for (int k = n; k >= 0; --k) { const float x = (float) k / (float) n; s.lineTo (cx - rx + 2 * rx * x, cy + ry * 0.45f + ry * 0.35f * std::sin (x * 12.0f + phase * 3.0f + 1.0f)); } s.closeSubPath(); break; }
            case shBolt: s.startNewSubPath (cx + 0.25f * rx, cy - ry); s.lineTo (cx - 0.7f * rx, cy + 0.12f * ry); s.lineTo (cx - 0.05f * rx, cy + 0.12f * ry); s.lineTo (cx - 0.35f * rx, cy + ry); s.lineTo (cx + 0.75f * rx, cy - 0.22f * ry); s.lineTo (cx + 0.1f * rx, cy - 0.22f * ry); s.closeSubPath(); break;
            case shDrop: s.startNewSubPath (cx, cy - ry); s.cubicTo (cx + rx * 1.1f, cy + ry * 0.1f, cx + rx * 0.8f, cy + ry, cx, cy + ry); s.cubicTo (cx - rx * 0.8f, cy + ry, cx - rx * 1.1f, cy + ry * 0.1f, cx, cy - ry); s.closeSubPath(); break;
            case shHeart: s.startNewSubPath (cx, cy + ry); s.cubicTo (cx - rx * 1.6f, cy - ry * 0.2f, cx - rx * 0.6f, cy - ry * 1.3f, cx, cy - ry * 0.4f); s.cubicTo (cx + rx * 0.6f, cy - ry * 1.3f, cx + rx * 1.6f, cy - ry * 0.2f, cx, cy + ry); s.closeSubPath(); break;
            case shLungs: s.addEllipse (cx - rx, cy - ry, rx * 0.9f, ry * 2); s.addEllipse (cx + rx * 0.1f, cy - ry, rx * 0.9f, ry * 2); break;
            case shBones:
                if (ry > rx) { s.addRoundedRectangle (cx - rx * 0.28f, cy - ry * 0.8f, rx * 0.56f, ry * 1.6f, rx * 0.2f); for (float sy : { -0.8f, 0.8f }) for (float sx : { -0.3f, 0.3f }) s.addEllipse (cx + sx * rx - rx * 0.3f, cy + sy * ry - ry * 0.13f, rx * 0.6f, ry * 0.26f); }
                else { s.addRoundedRectangle (cx - rx * 0.8f, cy - ry * 0.28f, rx * 1.6f, ry * 0.56f, ry * 0.2f); for (float sx : { -0.8f, 0.8f }) for (float sy : { -0.3f, 0.3f }) s.addEllipse (cx + sx * rx - rx * 0.13f, cy + sy * ry - ry * 0.3f, rx * 0.26f, ry * 0.6f); }
                break;
            case shLeaf: s.startNewSubPath (cx - rx, cy); s.quadraticTo (cx, cy - ry * 1.4f, cx + rx, cy); s.quadraticTo (cx, cy + ry * 1.4f, cx - rx, cy); s.closeSubPath(); break;
            default: s.addEllipse (cx - rx, cy - ry, rx * 2, ry * 2); break;
        }
        return s;
    }
    void drawBeing (Graphics& g, int i)
    {
        using namespace kk::worlds;
        const auto box = beingBox (i);
        const auto xf = xform (i);
        const auto& B = being (i);
        const auto& md = moods[(size_t) kingdom][(size_t) i];
        const float A = box.getWidth() / box.getHeight();
        const bool foc = focus[(size_t) kingdom] == i;
        auto col = Colour (B.colour);
        if (md.mutations > 0) col = col.withRotatedHue (0.17f * (float) md.mutations).withMultipliedSaturation (1.3f);
        // its habitat: a soft pool of light
        if (kingdom != kBody)
        {
            g.setGradientFill (ColourGradient (col.withAlpha (foc ? 0.22f : 0.1f), box.getCentreX(), box.getBottom() - 6, col.withAlpha (0.0f), box.getCentreX() + box.getWidth() * 0.6f, box.getBottom() - 6, true));
            g.fillEllipse (box.getX() - 10, box.getBottom() - box.getHeight() * 0.25f, box.getWidth() + 20, box.getHeight() * 0.4f);
        }
        // panic: speed lines behind it
        if (md.panic > 0.05f)
        {
            g.setColour (Colours::white.withAlpha (0.5f * md.panic));
            for (int k = 0; k < 5; ++k) { const float y = box.getY() + box.getHeight() * (0.3f + 0.1f * (float) k), x = box.getX() - 8 - 10.0f * std::sin (phase * 9.0f + (float) k); g.drawLine (x - 30 - 12.0f * (float) (k % 2), y, x, y, 2.0f); }
        }
        const auto body = kk::worldsui::placed (bodyPath (kingdom, i, A), xf);
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillPath (body, AffineTransform::translation (0, 6));
        g.setGradientFill (ColourGradient (col.withAlpha (0.42f).darker (0.4f), box.getX(), box.getY(), Colour (0xff0b0d14).withAlpha (0.92f), box.getRight(), box.getBottom(), false));
        g.fillPath (body);
        if (md.mutations > 0) drawMutations (g, i, xf, A, col, md.mutations);
        g.setColour (col.withAlpha (foc ? 0.95f : 0.7f)); g.strokePath (body, PathStrokeType (foc ? 2.2f : 1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        drawDetails (g, i, xf, A, col, md);
        // the parts: dashed outlines to colour in ... coloured = lit, glowing, alive
        const uint32 m = lit[(size_t) kingdom][(size_t) i];
        for (int pi = 0; pi < (int) B.parts.size(); ++pi)
        {
            const auto& P = B.parts[(size_t) pi];
            const bool on = (m >> pi) & 1u, hov = hoverBeing == i && hoverPart == pi;
            const float fl = flashBeing == i && flashPart == pi ? flash : 0.0f;
            float rate = P.shape == shHeart ? 7.0f + 10.0f * md.panic : P.shape == shLungs ? 1.6f + 3.0f * md.panic : 2.5f;
            const float pulse = on ? 1.0f + 0.05f * std::sin (phase * rate + (float) pi) + 0.25f * fl : 1.0f;
            const auto pp = kk::worldsui::placed (partPath (P, pulse), xf);
            const auto pc = md.mutations > 0 ? Colour (P.colour).withRotatedHue (0.17f * (float) md.mutations) : Colour (P.colour);
            if (on)
            {
                kk::worldsui::glowPath (g, pp, pc, 0.9f + fl);
                const auto bb = pp.getBounds();
                g.setGradientFill (ColourGradient (pc.brighter (0.5f), bb.getX(), bb.getY(), pc.darker (0.35f), bb.getRight(), bb.getBottom(), false));
                g.fillPath (pp);
                g.setColour (Colours::white.withAlpha (0.8f)); g.strokePath (pp, PathStrokeType (1.4f));
                if (P.shape == shRing) for (int w = 0; w < 3; ++w)
                {
                    const float f = std::fmod (phase * 0.5f + (float) w / 3.0f, 1.0f);
                    const float rx = bb.getWidth() * (0.6f + 1.2f * f), ry = bb.getHeight() * (0.6f + 1.2f * f);
                    g.setColour (pc.withAlpha (0.6f * (1.0f - f))); g.drawEllipse (bb.getCentreX() - rx, bb.getCentreY() - ry, rx * 2, ry * 2, 1.6f);
                }
            }
            else
            {
                Path dashed; const float dl[] { 5.0f, 4.0f };
                PathStrokeType (hov ? 2.0f : 1.3f).createDashedStroke (dashed, pp, dl, 2);
                if (hov) { g.setColour (pc.withAlpha (0.25f)); g.fillPath (pp); }
                g.setColour (pc.withAlpha (hov ? 1.0f : 0.55f + 0.2f * std::sin (phase * 2.0f + (float) pi))); g.fillPath (dashed);
            }
            if (hov || fl > 0.3f)
            {
                const auto bb = pp.getBounds();
                const String s = String (P.name) + "  -  " + P.trait;
                g.setFont (kk::modern::font (12.0f, true, 0.08f));
                const float tw = GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 18;
                const Rectangle<float> tag (jlimit (box.getX() - 20, box.getRight() - tw + 20, bb.getCentreX() - tw * 0.5f), bb.getY() - 28, tw, 22);
                g.setColour (Colour (0xee0a0c12)); g.fillRoundedRectangle (tag, 11);
                g.setColour (pc); g.drawRoundedRectangle (tag, 11, 1.2f);
                g.setColour (Colours::white); g.drawText (s, tag, Justification::centred);
            }
        }
        // shy: a plant hides in the mist ...
        if (md.shy > 0.05f && kingdom == kNature)
            for (int k = 0; k < 4; ++k)
            {
                const float y = box.getY() + box.getHeight() * (0.25f + 0.2f * (float) k), dx = 20.0f * std::sin (phase * 0.6f + (float) k);
                g.setGradientFill (ColourGradient (Colour (0xffc8d8ff).withAlpha (0.28f * md.shy), box.getCentreX() + dx, y, Colour (0xffc8d8ff).withAlpha (0.0f), box.getRight() + dx, y, true));
                g.fillEllipse (box.getX() - 20 + dx, y - box.getHeight() * 0.14f, box.getWidth() + 40, box.getHeight() * 0.28f);
            }
        // ... an animal behind a bush
        if (md.shy > 0.05f && kingdom == kAnimals)
        {
            const float h = box.getHeight() * 0.5f * md.shy;
            Random rr (i + 11);
            for (int k = 0; k < 9; ++k)
            {
                const float x = box.getX() - 6 + box.getWidth() * (0.06f + 0.11f * (float) k), r = box.getHeight() * (0.12f + 0.06f * rr.nextFloat()) * (0.4f + 0.6f * md.shy);
                const float y = box.getBottom() + 6 - h * (0.55f + 0.4f * rr.nextFloat()) + 2.0f * std::sin (phase * 1.5f + (float) k);
                const auto lc = Colour (0xff2f9f58).withMultipliedBrightness (0.7f + 0.5f * rr.nextFloat());
                g.setGradientFill (ColourGradient (lc.brighter (0.4f), x, y - r, lc.darker (0.6f), x, y + r * 1.4f, false));
                g.fillEllipse (x - r, y - r, r * 2.2f, r * 2.4f);
                g.setColour (Colour (0xffb4ffc8).withAlpha (0.25f)); g.drawEllipse (x - r, y - r, r * 2.2f, r * 2.4f, 1.0f);
                for (int l = 0; l < 3; ++l) { const float a = rr.nextFloat() * MathConstants<float>::twoPi; g.setColour (Colour (0xff8fffb0).withAlpha (0.35f)); g.drawLine (x, y, x + std::cos (a) * r * 0.7f, y + std::sin (a) * r * 0.7f, 1.0f); }
            }
            g.setColour (Colours::white.withAlpha (0.7f * md.shy)); g.setFont (kk::modern::font (16.0f, true, 0.3f));
            g.drawText ("...", Rectangle<float> (box.getCentreX() - 30, box.getBottom() - h - 34, 60, 20), Justification::centred);
        }
        if (mutFlash[(size_t) i] > 0)
        {
            const float f = mutFlash[(size_t) i], r = box.getHeight() * (0.4f + 0.8f * (1.0f - f));
            g.setColour (Colour (0xffd16bff).withAlpha (0.7f * f)); g.drawEllipse (box.getCentreX() - r, box.getCentreY() - r, r * 2, r * 2, 3.0f);
        }
        // name + its mood
        if (kingdom != kBody)
        {
            const auto nr = Rectangle<float> (box.getX(), box.getBottom() + 8, box.getWidth(), 20);
            g.setColour (Colours::white.withAlpha (foc ? 1.0f : 0.75f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
            g.drawText (B.name, nr, Justification::centred);
            String mood; Colour mc;
            if (md.panic > 0.5f) { mood = "PANIC!"; mc = Colour (0xffff3b5c); }
            else if (md.shy > 0.5f) { mood = "SHY..."; mc = Colour (0xff7fe0ff); }
            else if (md.mutations > 0) { mood = "MUTANT"; for (int k = 1; k < md.mutations; ++k) mood << "+"; mc = Colour (0xffd16bff); }
            if (mood.isNotEmpty())
            {
                const Rectangle<float> b2 (box.getRight() - 90, box.getY() - 4, 92, 24);
                g.setColour (mc.withAlpha (0.2f)); g.fillRoundedRectangle (b2, 12); g.setColour (mc); g.drawRoundedRectangle (b2, 12, 1.4f);
                g.setFont (kk::modern::font (12.5f, true, 0.2f)); g.drawText (mood, b2, Justification::centred);
            }
        }
    }
    void drawMutations (Graphics& g, int i, const AffineTransform& xf, float A, Colour col, int muts)
    {
        Random rr (i * 31 + 5);
        // extra limbs grow out of it, wobbling
        for (int k = 0; k < muts + 1; ++k)
        {
            const float a = MathConstants<float>::pi * (0.15f + 0.7f * rr.nextFloat()) + (k % 2 ? MathConstants<float>::pi : 0.0f) * 0.35f;
            const float x0 = 0.5f + std::cos (a) * 0.15f / A * 1.4f, y0 = 0.6f + std::sin (a) * 0.12f;
            const float x1 = 0.5f + std::cos (a) * 0.42f / A * 1.4f + 0.03f * std::sin (phase * 3.0f + (float) k), y1 = 0.62f + std::sin (a) * 0.4f + 0.03f * std::cos (phase * 2.5f + (float) k);
            Path t; t.startNewSubPath (x0, y0); t.quadraticTo ((x0 + x1) * 0.5f + 0.07f * std::sin (phase * 2.0f + (float) k), (y0 + y1) * 0.5f - 0.1f, x1, y1);
            const auto lp = kk::worldsui::placed (t, xf);
            const auto lc = col.withRotatedHue (0.25f);
            g.setColour (lc.darker (0.5f)); g.strokePath (lp, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (lc); g.strokePath (lp, PathStrokeType (5.5f, PathStrokeType::curved, PathStrokeType::rounded));
            float ex = x1, ey = y1; xf.transformPoint (ex, ey);
            g.setColour (lc.brighter (0.6f)); g.fillEllipse (ex - 5, ey - 5, 10, 10);
        }
        for (int k = 0; k < muts + 1; ++k)
        {
            float x = 0.3f + 0.4f * rr.nextFloat(), y = 0.35f + 0.3f * rr.nextFloat();
            xf.transformPoint (x, y);
            const float r = 6.0f + 3.0f * rr.nextFloat(), blink = std::abs (std::sin (phase * 0.7f + (float) k)) > 0.97f ? 0.2f : 1.0f;
            g.setColour (Colours::white); g.fillEllipse (x - r, y - r * blink, r * 2, r * 2 * blink);
            g.setColour (Colours::black); g.fillEllipse (x - r * 0.45f + 1.5f * std::sin (phase), y - r * 0.45f * blink, r * 0.9f, r * 0.9f * blink);
        }
    }
    void drawDetails (Graphics& g, int i, const AffineTransform& xf, float A, Colour col, const kk::worlds::Mood& md)
    {
        auto pt = [&] (float x, float y) { xf.transformPoint (x, y); return Point<float> (x, y); };
        auto eye = [&] (float x, float y, float r)
        {
            const auto c = pt (x, y);
            const bool closed = md.shy > 0.5f;
            if (closed) { g.setColour (Colours::white.withAlpha (0.8f)); g.drawLine (c.x - r, c.y, c.x + r, c.y, 2.0f); return; }
            const float big = 1.0f + 0.6f * md.panic;
            g.setColour (Colours::white); g.fillEllipse (c.x - r * big, c.y - r * big, r * 2 * big, r * 2 * big);
            g.setColour (Colours::black); g.fillEllipse (c.x - r * 0.5f, c.y - r * 0.5f, r, r);
        };
        g.setColour (Colours::white.withAlpha (0.55f));
        if (kingdom == kk::worlds::kAnimals)
        {
            switch (i)
            {
                case 0: eye (0.80f, 0.28f, 4.0f); break;
                case 1: eye (0.74f, 0.57f, 4.0f); { g.setColour (Colours::white.withAlpha (0.4f)); const auto a = pt (0.66f, 0.66f), b = pt (0.88f, 0.62f); g.drawLine (a.x, a.y, b.x, b.y, 1.5f); }
                        for (int k = 0; k < 3; ++k) { const auto s = pt (0.62f + 0.03f * (float) (k - 1), 0.30f); const float h = 14.0f + 6.0f * std::sin (phase * 4.0f + (float) k); g.setColour (Colour (0xff7fe0ff).withAlpha (0.6f)); g.drawLine (s.x, s.y, s.x + (float) (k - 1) * 8.0f, s.y - h, 2.0f); } break;
                case 2: eye (0.745f, 0.35f, 3.5f); break;
                case 3:
                {
                    Random rr (77);
                    for (int b = 0; b < 16; ++b)
                    {
                        const float bx = 0.08f + 0.84f * rr.nextFloat(), by = 0.1f + 0.8f * rr.nextFloat();
                        const float x = bx + 0.02f * std::sin (phase * (6.0f + (float) b) + (float) b), y = by + 0.03f * std::cos (phase * (5.0f + (float) b * 0.7f) + (float) b);
                        if (std::abs (x - 0.52f) < 0.15f && std::abs (y - 0.5f) < 0.2f) continue;
                        const auto c = pt (x, y); const float flap = 4.0f + 3.0f * std::abs (std::sin (phase * 40.0f + (float) b));
                        g.setColour (Colours::white.withAlpha (0.45f)); g.fillEllipse (c.x - 6, c.y - flap - 4, 7, flap); g.fillEllipse (c.x, c.y - flap - 4, 7, flap);
                        g.setColour (Colour (0xff1a1a10)); g.drawLine (c.x - 3, c.y - 3, c.x - 3, c.y + 3, 1.5f); g.drawLine (c.x + 2, c.y - 3, c.x + 2, c.y + 3, 1.5f);
                    }
                    for (int r = 0; r < 3; ++r) { const auto a = pt (0.44f, 0.38f + 0.12f * (float) r), b = pt (0.60f, 0.38f + 0.12f * (float) r); g.setColour (col.withAlpha (0.4f)); g.drawLine (a.x, a.y, b.x, b.y, 1.2f); }
                    break;
                }
                case 4:
                    eye (0.615f, 0.24f, 3.5f); eye (0.69f, 0.24f, 3.5f);
                    for (int k = 0; k < 3; ++k) { const auto a = pt (0.73f, 0.31f + 0.02f * (float) k), b = pt (0.86f, 0.27f + 0.04f * (float) k); g.setColour (Colours::white.withAlpha (0.6f)); g.drawLine (a.x, a.y, b.x, b.y, 1.2f); }
                    break;
                default:
                {
                    eye (0.88f, 0.25f, 3.5f);
                    const auto t0 = pt (0.94f, 0.30f), t1 = pt (0.99f + 0.01f * std::sin (phase * 12.0f), 0.31f);
                    g.setColour (Colour (0xffff3b5c)); g.drawLine (t0.x, t0.y, t1.x, t1.y, 2.0f);
                    break;
                }
            }
        }
        else if (kingdom == kk::worlds::kNature)
        {
            if (i == 2)   // fish and bubbles in the ocean
                for (int k = 0; k < 4; ++k) { const auto c = pt (std::fmod (0.15f + 0.22f * (float) k + phase * 0.02f * (float) (k + 1), 1.0f), 0.5f + 0.08f * (float) (k % 2)); g.setColour (Colours::white.withAlpha (0.35f)); g.fillEllipse (c.x - 7, c.y - 3, 14, 6); }
            if (i == 3)   // rain
                for (int k = 0; k < 18; ++k) { const float x = 0.1f + 0.045f * (float) k, y = 0.36f + std::fmod (phase * 0.4f + (float) k * 0.37f, 0.6f); const auto a = pt (x, y), b = pt (x - 0.01f, y + 0.06f); g.setColour (Colour (0xff7fe0ff).withAlpha (0.45f)); g.drawLine (a.x, a.y, b.x, b.y, 1.4f); }
            if (i == 4)   // wind
                for (int k = 0; k < 3; ++k) { Path w; const float y = 0.3f + 0.1f * (float) k, s = std::fmod (phase * 0.15f + (float) k * 0.3f, 1.0f); w.startNewSubPath (s * 0.6f, y); w.quadraticTo (s * 0.6f + 0.15f, y - 0.05f, s * 0.6f + 0.3f, y); g.setColour (Colours::white.withAlpha (0.3f)); g.strokePath (kk::worldsui::placed (w, xf), PathStrokeType (1.5f)); }
            if (i == 0)   // the ground
            { const auto a = pt (0.05f, 0.94f), b = pt (0.95f, 0.94f); g.setColour (col.withAlpha (0.35f)); g.drawLine (a.x, a.y, b.x, b.y, 1.5f); }
        }
        else
        {
            // ribs, spine and a face like on the school chart
            Path d;
            for (int r = 0; r < 4; ++r) { const float y = 0.27f + 0.05f * (float) r; d.startNewSubPath (0.38f, y + 0.02f); d.quadraticTo (0.5f, y - 0.03f, 0.62f, y + 0.02f); }
            d.startNewSubPath (0.5f, 0.22f); d.lineTo (0.5f, 0.55f);
            g.setColour (Colours::white.withAlpha (0.18f)); g.strokePath (kk::worldsui::placed (d, xf), PathStrokeType (1.4f));
            ignoreUnused (A);
        }
    }
    void drawStage (Graphics& g)
    {
        const auto s = stage().toFloat();
        const bool night = kk::theme().night;
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (s, 18); g.reduceClipRegion (clip);
        const auto top = kingdom == kk::worlds::kBody ? Colour (0xff0d1424) : kingdom == kk::worlds::kNature ? Colour (0xff0b1a1c) : Colour (0xff0c1a14);
        g.setGradientFill (ColourGradient (top, s.getX(), s.getY(), Colour (0xff05070a), s.getX(), s.getBottom(), false)); g.fillRect (s);
        // a faint living grid of cells
        g.setColour (Colour (0xff36ff9a).withAlpha (night ? 0.05f : 0.07f));
        for (float x = s.getX() + 20; x < s.getRight(); x += 40) g.drawVerticalLine ((int) x, s.getY(), s.getBottom());
        for (float y = s.getY() + 20; y < s.getBottom(); y += 40) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
        Random rr (5);
        for (int k = 0; k < 40; ++k) { const float x = s.getX() + rr.nextFloat() * s.getWidth(), y = s.getY() + std::fmod (rr.nextFloat() * s.getHeight() - phase * 6.0f * (0.5f + rr.nextFloat()), s.getHeight()); g.setColour (Colour (0xff36ff9a).withAlpha (0.12f)); g.fillEllipse (x, y < s.getY() ? y + s.getHeight() : y, 2.5f, 2.5f); }
        if (kingdom == kk::worlds::kBody) drawAnatomyLabels (g);
        for (int i = 0; i < numBeings(); ++i) drawBeing (g, i);
        g.setColour (Colours::white.withAlpha (0.4f)); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText (kingdom == kk::worlds::kBody ? "click an organ = colour it in.   right-click = wash it off" : "click a part = colour it in.   right-click = wash it off.   double-click = mutate",
                    s.reduced (16, 8).withHeight (16), Justification::centredLeft);
    }
    void drawAnatomyLabels (Graphics& g)
    {
        const auto& B = being (0); const auto xf = xform (0);
        for (int pi = 0; pi < (int) B.parts.size(); ++pi)
        {
            const auto& P = B.parts[(size_t) pi];
            const auto r = labelRect (pi);
            const bool on = (lit[2][0] >> pi) & 1u, hov = hoverPart == pi;
            float x = P.cx, y = P.cy; xf.transformPoint (x, y);
            const bool left = r.getRight() < x;
            const Point<float> a (left ? r.getRight() : r.getX(), r.getCentreY());
            g.setColour (Colour (P.colour).withAlpha (on ? 0.9f : 0.35f)); g.drawLine (a.x, a.y, x, y, on ? 1.8f : 1.0f);
            g.fillEllipse (x - 3, y - 3, 6, 6);
            g.setColour (Colour (P.colour).withAlpha (on ? 0.22f : hov ? 0.12f : 0.05f)); g.fillRoundedRectangle (r, 10);
            g.setColour (Colour (P.colour).withAlpha (on ? 1.0f : 0.45f)); g.drawRoundedRectangle (r, 10, on ? 1.8f : 1.0f);
            g.setColour (Colours::white.withAlpha (on ? 1.0f : 0.75f)); g.setFont (kk::modern::font (15.0f, true, 0.25f));
            g.drawText (P.name, r.withTrimmedBottom (18).reduced (12, 0), left ? Justification::centredRight : Justification::centredLeft);
            g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (11.0f, true, 0.05f));
            g.drawText (P.trait, r.withTrimmedTop (24).reduced (12, 0), left ? Justification::centredRight : Justification::centredLeft);
        }
    }
    void drawInfo (Graphics& g)
    {
        const auto a = panelTop().toFloat();
        const int f = focus[(size_t) kingdom];
        const auto& B = being (f); const auto& md = moods[(size_t) kingdom][(size_t) f];
        g.setColour (Colour (B.colour).withMultipliedBrightness (kk::theme().night ? 1.0f : 0.55f)); g.setFont (kk::modern::font (22.0f, true, 0.15f));
        g.drawText (B.name, a.withHeight (28), Justification::centredLeft);
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (12.0f, true, 0.03f));
        g.drawText (B.hint, a.withTrimmedTop (28).withHeight (18), Justification::centredLeft);
        float y = a.getY() + 56;
        g.setColour (kk::theme().text.withAlpha (0.85f)); g.setFont (kk::modern::font (11.5f, true, 0.3f));
        g.drawText ("COLOURED IN", Rectangle<float> (a.getX(), y, a.getWidth(), 16), Justification::centredLeft); y += 22;
        const uint32 m = lit[(size_t) kingdom][(size_t) f];
        if (m == 0) { g.setColour (kk::theme().dim); g.setFont (kk::modern::font (12.0f, true, 0.03f)); g.drawText ("nothing yet - click a part", Rectangle<float> (a.getX(), y, a.getWidth(), 18), Justification::centredLeft); y += 22; }
        for (int pi = 0; pi < (int) B.parts.size(); ++pi)
        {
            if (! ((m >> pi) & 1u)) continue;
            const auto& P = B.parts[(size_t) pi];
            g.setColour (Colour (P.colour)); g.fillEllipse (a.getX() + 2, y + 4, 10, 10);
            g.setColour (kk::theme().text); g.setFont (kk::modern::font (12.5f, true, 0.12f));
            g.drawText (P.name, Rectangle<float> (a.getX() + 20, y, 110, 18), Justification::centredLeft);
            g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.5f, true, 0.03f));
            g.drawText (P.trait, Rectangle<float> (a.getX() + 128, y, a.getWidth() - 128, 18), Justification::centredLeft);
            y += 21;
        }
        y += 8;
        String mood = "calm - it lets you colour it"; Colour mc = kk::theme().dim;
        if (md.panic > 0.5f) { mood = "PANIC - its sounds go wild: hotter, brighter, shaking"; mc = Colour (0xffff3b5c); }
        else if (md.shy > 0.5f) { mood = "SHY - it hides: softer, darker, further away"; mc = Colour (0xff7fe0ff); }
        else if (md.mutations > 0) { mood = "MUTANT - a new mutation, torn in two"; mc = Colour (0xffd16bff); }
        g.setColour (mc); g.setFont (kk::modern::font (12.0f, true, 0.05f));
        g.drawFittedText (mood, Rectangle<float> (a.getX(), y, a.getWidth(), 34).toNearestInt(), Justification::topLeft, 2, 0.85f);
        y += 40;
        g.setColour (kk::theme().dim.withAlpha (0.9f)); g.setFont (kk::modern::font (11.0f, true, 0.03f));
        if (y < a.getBottom() - 30)
            g.drawFittedText (kingdom == kk::worlds::kBody ? "every organ has its own sound:\nheart = pulse / bass,  lungs = breath,  throat = a vowel,\nbrain = sparks,  bones = hollow bone,  blood = liquid"
                                                           : "move fast near it = PANIC\ncome slowly = SHY, it hides\nstay with it, or double-click = it MUTATES",
                              Rectangle<float> (a.getX(), y, a.getWidth(), a.getBottom() - y).toNearestInt(), Justification::topLeft, 4, 0.85f);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        tickBase();
        if (flash > 0) flash = jmax (0.0f, flash - 0.04f);
        for (auto& f : mutFlash) if (f > 0) f = jmax (0.0f, f - 0.03f);
        const auto now = Time::getMillisecondCounter();
        if (now - lastMs > 60) speed *= 0.6f;
        const float dt = 1.0f / 30.0f;
        for (int i = 0; i < numBeings(); ++i)
        {
            const auto b = beingBox (i);
            const float rad = jmax (b.getWidth(), b.getHeight()) * (kingdom == kk::worlds::kBody ? 0.45f : 0.62f);
            const float near = mouseIn ? jlimit (0.0f, 1.0f, 1.0f - mpos.getDistanceFrom (b.getCentre()) / rad) : 0.0f;
            auto& md = moods[(size_t) kingdom][(size_t) i];
            const int ev = kk::worlds::react (md, near, speed, dt);
            if (ev == kk::worlds::reactNone || now - lastEvent < 700) continue;
            lastEvent = now;
            if (ev == kk::worlds::reactMutate) { mutFlash[(size_t) i] = 1.0f; focus[(size_t) kingdom] = i; reroll (true); note = String (being (i).name) + " MUTATED - a new sound, torn in two"; }
            else if (ev == kk::worlds::reactPanic) { focus[(size_t) kingdom] = i; reroll (true); note = String (being (i).name) + " PANICS - its sounds go wild"; }
            else if (ev == kk::worlds::reactShy) { focus[(size_t) kingdom] = i; reroll (false); note = String (being (i).name) + " is SHY - softer, darker"; }
        }
        repaint (stage()); repaint (panel()); repaint (strip());
    }

    HotButton tabA { lnf }, tabB { lnf }, tabC { lnf };
    std::array<HotButton*, 3> tabs { &tabA, &tabB, &tabC };
    int kingdom = 0, lastPart = -1, hoverBeing = -1, hoverPart = -1, flashBeing = -1, flashPart = -1;
    std::array<std::array<uint32, 8>, 3> lit {};
    std::array<std::array<kk::worlds::Mood, 8>, 3> moods {};
    std::array<int, 3> focus { 0, 0, 0 };
    std::array<float, 8> mutFlash {};
    Point<float> mpos; bool mouseIn = false; float speed = 0, flash = 0;
    uint32 lastMs = 0, lastEvent = 0;
};

//==============================================================================
// GARAGE (FLEX): ARSENAL / CARS / HARBOUR / HANGAR. A ladder from the worst to the most expensive (invented names, made-up credits);
// pick one = a roll of 10 sounds in its character (a higher tier = rarer, richer recipes). Click a PART of the drawn item =
// the chosen sound is re-shaped that way (barrel / trigger / magazine / scope; engine / exhaust / tyres / horn / turbo ...).
class GaragePage : public WorldPage
{
public:
    GaragePage (KeysKillaProcessor& p, KKLookAndFeel& l) : WorldPage (p, l, Colour (0xffffc83d))
    {
        for (int k = 0; k < kk::worlds::numGarage; ++k)
        {
            auto& b = *tabs[(size_t) k];
            b.setButtonText (kk::worlds::garageName (k)); b.framed = true; b.onClick = [this, k] { setTab (k); }; addAndMakeVisible (b);
        }
        tabs[0]->setTooltip ("Cartoon blasters: hits, snaps, metallic transients, booms");
        tabs[1]->setTooltip ("Cars: engine rumbles, 808s and basses, revs, screeching tyres");
        tabs[2]->setTooltip ("Boats: wide pads, bells, water, horns");
        tabs[3]->setTooltip ("Planes: risers, swooshes, airy leads, turbines");
        tabsW = 4 * 138;
        seed = 11;
        setTab (1, false);
        setMouseCursor (MouseCursor::PointingHandCursor);
    }
    // debug / snapshot: 82 = CARS with a roll and a re-shaped sound, 83 = ARSENAL
    void debugShow (int view)
    {
        if (view == 83) { setTab (0, false); setTier (4, false); sel = 2; reshape (0, false); reshape (3, false); }
        else            { setTab (1, false); setTier (3, false); sel = 1; reshape (4, false); reshape (0, false); }
        phase = 1.7f; flash = 0.7f;
    }
    void setTier (int t, bool audition = true) { tiers[(size_t) tab] = jlimit (0, 5, t); rungFlash = 1.0f; reroll (audition); }
    void reshape (int part, bool audition = true)
    {
        if (chosen() == nullptr) return;
        const int tier = tiers[(size_t) tab];
        const auto P = kk::worlds::garagePart (tab, part, tier);
        replaceSel (kk::worlds::applyPart (*chosen(), tab, part, tier), audition);
        touched.add (P.name); touched.removeDuplicates (false);
        flashPart = part; flash = 1.0f;
        note = String (P.name) + ": " + P.hint;
    }
    void paint (Graphics& g) override
    {
        pageBackdrop (g, *this);
        drawHeader (g, "GARAGE", "climb the ladder from the worst to the most expensive - ten sounds in its character.  click a PART of it = the chosen sound is re-shaped");
        drawStage (g);
        drawPanel (g);
        drawInfo (g);
        drawStrip (g);
    }
    void layoutTabs() override { layoutTabRow ({ tabs[0], tabs[1], tabs[2], tabs[3] }, 130); }
    void mouseMove (const MouseEvent& e) override
    {
        int hr = -1, hp = -1;
        for (int i = 0; i < 6; ++i) if (rung (i).contains (e.position)) hr = i;
        if (showroom().contains (e.position)) hp = partUnder (e.position);
        if (hr != hoverRung || hp != hoverPart) { hoverRung = hr; hoverPart = hp; repaint (stage()); }
        stripHover (e.position);
    }
    void mouseExit (const MouseEvent&) override { hoverRung = hoverPart = hoverCard = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        if (stripDown (e)) return;
        for (int i = 0; i < 6; ++i) if (rung (i).contains (e.position)) { setTier (i); note = String (kk::worlds::items (tab)[(size_t) i].name) + " - " + kk::worlds::rarity (i); return; }
        if (showroom().contains (e.position)) if (const int pp = partUnder (e.position); pp >= 0) reshape (pp);
    }
private:
    void setTab (int k, bool audition = true)
    {
        tab = jlimit (0, 3, k);
        for (int i = 0; i < 4; ++i) { tabs[(size_t) i]->selected = i == tab; tabs[(size_t) i]->repaint(); }
        hoverPart = hoverRung = -1;
        reroll (audition);
    }
    void reroll (bool audition) override { touched.clear(); setRoll (kk::worlds::garageRoll (tab, tiers[(size_t) tab], seed), audition); }
    Rectangle<float> ladder() const { return stage().toFloat().withHeight (150).reduced (14, 12); }
    Rectangle<float> rung (int i) const { const auto l = ladder(); const float w = l.getWidth() / 6.0f; return Rectangle<float> (l.getX() + (float) i * w, l.getY(), w, l.getHeight()).reduced (5, 0); }
    Rectangle<float> showroom() const { return stage().toFloat().withTrimmedTop (176).reduced (14, 10); }
    Rectangle<float> itemBox() const
    {
        const auto s = showroom();
        const float w = jmin (s.getWidth() * 0.66f, (s.getHeight() - 112) * 2.1f), h = w / 2.1f;
        return { s.getCentreX() - w * 0.5f + 40, s.getY() + (s.getHeight() - h) * 0.5f + 2, w, h };
    }
    int partUnder (Point<float> p) const
    {
        const auto b = itemBox(); const auto ms = markers();
        for (int i = 0; i < (int) ms.size(); ++i) if (ms[(size_t) i].contains (p)) return i;
        return kk::worlds::garagePartAt (tab, tiers[(size_t) tab], (p.x - b.getX()) / b.getWidth(), (p.y - b.getY()) / b.getHeight());
    }
    Point<float> partCentre (int i) const { const auto b = itemBox(); const auto P = kk::worlds::garagePart (tab, i, tiers[(size_t) tab]); return { b.getX() + P.cx * b.getWidth(), b.getY() + P.cy * b.getHeight() }; }
    // the part labels: above or below the item, spread so they never overlap
    std::vector<Rectangle<float>> markers() const
    {
        const auto b = itemBox(); const auto s = showroom();
        const int n = (int) kk::worlds::garageParts (tab).size();
        std::vector<Rectangle<float>> out ((size_t) n);
        for (int up = 0; up < 2; ++up)
        {
            std::vector<int> ids;
            for (int i = 0; i < n; ++i) if ((partCentre (i).y < b.getY() + b.getHeight() * 0.42f) == (up == 1)) ids.push_back (i);
            std::sort (ids.begin(), ids.end(), [this] (int a, int c) { return partCentre (a).x < partCentre (c).x; });
            const float w = 118, gap = 8, lo = s.getX() + 4, hi = s.getRight() - w - 4;
            std::vector<float> xs;
            for (int i : ids) xs.push_back (jlimit (lo, hi, partCentre (i).x - w * 0.5f));
            for (size_t k = 1; k < xs.size(); ++k) xs[k] = jmax (xs[k], xs[k - 1] + w + gap);
            for (int k = (int) xs.size() - 1; k >= 0; --k) xs[(size_t) k] = jmin (xs[(size_t) k], k == (int) xs.size() - 1 ? hi : xs[(size_t) k + 1] - w - gap);
            const float y = up == 1 ? jmax (s.getY() + 54, b.getY() - 42) : jmin (s.getBottom() - 46, b.getBottom() + 16);
            for (size_t k = 0; k < ids.size(); ++k) out[(size_t) ids[k]] = { xs[k], y, w, 26 };
        }
        return out;
    }
    // ---- the drawn items (cartoon vector art in a 0..1 box, A = aspect)
    void shape (Graphics& g, const Path& unit, const AffineTransform& xf, Colour c, float outline)
    {
        const auto p = kk::worldsui::placed (unit, xf);
        const auto b = p.getBounds();
        g.setGradientFill (ColourGradient (c.brighter (0.35f), b.getX(), b.getY(), c.darker (0.45f), b.getX(), b.getBottom(), false)); g.fillPath (p);
        g.setColour (Colour (0xff07080c)); g.strokePath (p, PathStrokeType (outline, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.22f)); g.strokePath (p, PathStrokeType (1.0f), AffineTransform::translation (0, 1.2f));
    }
    void drawItem (Graphics& g, int tb, int tier, Rectangle<float> box, bool big)
    {
        using kk::worldsui::thick;
        const auto xf = kk::worldsui::unitTo (box);
        const float A = box.getWidth() / box.getHeight(), t = (float) tier / 5.0f, ol = big ? 3.0f : 1.4f;
        auto pt = [&] (float x, float y) { xf.transformPoint (x, y); return Point<float> (x, y); };
        auto ell = [] (Path& p, float cx, float cy, float rx, float ry) { p.addEllipse (cx - rx, cy - ry, rx * 2, ry * 2); };
        auto rr = [] (float x, float y, float w, float h, float c) { Path p; p.addRoundedRectangle (x, y, w, h, c); return p; };
        auto glowAt = [&] (float x, float y, float r, Colour c, float a) { const auto q = pt (x, y); const float R = r * box.getHeight(); g.setGradientFill (ColourGradient (c.withAlpha (a), q.x, q.y, c.withAlpha (0.0f), q.x + R, q.y, true)); g.fillEllipse (q.x - R, q.y - R, R * 2, R * 2); };
        auto line = [&] (float x1, float y1, float x2, float y2, Colour c, float w) { const auto a = pt (x1, y1), b = pt (x2, y2); g.setColour (c); g.drawLine (a.x, a.y, b.x, b.y, w); };
        if (tb == kk::worlds::gCars)
        {
            static const uint32 cols[] { 0xff9a5b34, 0xffc9b48a, 0xff1fd1c1, 0xffff3b5c, 0xffffc83d, 0xff1c1d26 };
            const auto c = Colour (cols[tier]);
            const float roofY = 0.2f + 0.17f * t, hoodY = 0.47f + 0.05f * t, rearTop = 0.36f + 0.14f * t;
            if (tier >= 2) glowAt (0.5f, 0.84f, 0.5f, tier == 5 ? Colour (0xffb15cff) : Colour (0xff22d3ee), big ? 0.35f : 0.25f);
            g.setColour (Colours::black.withAlpha (0.4f)); g.fillEllipse (Rectangle<float> (pt (0.04f, 0.84f), pt (0.98f, 0.94f)));
            if (tier >= 3)   // spoiler
            {
                Path sp; sp.startNewSubPath (0.01f, rearTop - 0.10f); sp.lineTo (0.17f, rearTop - 0.12f); sp.lineTo (0.17f, rearTop - 0.07f); sp.lineTo (0.02f, rearTop - 0.05f); sp.closeSubPath();
                thick (sp, 0.08f, rearTop - 0.07f, 0.09f, rearTop + 0.02f, 0.012f, A);
                shape (g, sp, xf, tier == 5 ? Colour (0xffffc83d) : c.darker (0.3f), ol);
            }
            Path body; body.startNewSubPath (0.03f, 0.75f); body.lineTo (0.03f, rearTop + 0.06f); body.lineTo (0.07f, rearTop); body.lineTo (0.2f + 0.08f * t, roofY); body.lineTo (0.5f + 0.04f * t, roofY);
            body.lineTo (0.68f + 0.03f * t, hoodY); body.lineTo (0.94f, hoodY + 0.05f + 0.03f * t); body.lineTo (0.985f, 0.63f); body.lineTo (0.98f, 0.76f); body.closeSubPath();
            shape (g, body.createPathWithRoundedCorners (0.025f), xf, c, ol);
            Path win; win.startNewSubPath (0.215f + 0.08f * t, roofY + 0.035f); win.lineTo (0.49f + 0.04f * t, roofY + 0.035f); win.lineTo (0.645f + 0.03f * t, hoodY - 0.005f); win.lineTo (0.17f + 0.07f * t, hoodY - 0.005f); win.closeSubPath();
            const auto wp = kk::worldsui::placed (win.createPathWithRoundedCorners (0.012f), xf);
            g.setGradientFill (ColourGradient (Colour (0xff9be7ff).withAlpha (0.85f), wp.getBounds().getX(), wp.getBounds().getY(), Colour (0xff0e2236), wp.getBounds().getRight(), wp.getBounds().getBottom(), false)); g.fillPath (wp);
            g.setColour (Colour (0xff07080c)); g.strokePath (wp, PathStrokeType (ol * 0.7f));
            line (0.40f + 0.03f * t, roofY + 0.03f, 0.40f + 0.03f * t, hoodY, Colour (0xff07080c), ol * 1.3f);
            if (tier == 0)
            {
                Random rnd (3);
                for (int k = 0; k < 6; ++k) { const auto q = pt (0.12f + 0.75f * rnd.nextFloat(), hoodY + 0.08f + 0.15f * rnd.nextFloat()); const float r = (big ? 7.0f : 3.0f) * (0.6f + rnd.nextFloat()); g.setColour (Colour (0xff5a2e14).withAlpha (0.8f)); g.fillEllipse (q.x - r, q.y - r * 0.7f, r * 2, r * 1.4f); }
                line (0.55f, 0.56f, 0.62f, 0.66f, Colour (0xffd8d0b0), big ? 6.0f : 2.5f); line (0.62f, 0.56f, 0.55f, 0.66f, Colour (0xffd8d0b0), big ? 6.0f : 2.5f);
            }
            if (tier == 4 || tier == 5) line (0.06f, 0.6f, 0.95f, 0.6f, Colour (tier == 5 ? 0xffffc83d : 0xfffff2b0), big ? 3.0f : 1.4f);
            { Path sc = rr (0.75f, hoodY + 0.0f, 0.10f, 0.035f, 0.012f); shape (g, sc, xf, tier >= 3 ? Colour (0xff2a2d36) : c.darker (0.25f), ol * 0.7f); }
            // lights, exhaust, horn grille
            glowAt (0.955f, hoodY + 0.09f + 0.02f * t, 0.09f, Colour (0xfffff2a0), 0.8f);
            g.setColour (Colour (0xfffff6c8)); g.fillEllipse (Rectangle<float> (pt (0.935f, hoodY + 0.07f + 0.02f * t), pt (0.975f, hoodY + 0.11f + 0.02f * t)));
            g.setColour (Colour (0xffff3b5c)); g.fillRoundedRectangle (Rectangle<float> (pt (0.03f, rearTop + 0.07f), pt (0.055f, rearTop + 0.13f)), 2.0f);
            { Path ex = rr (0.0f, 0.69f, 0.06f, 0.035f, 0.012f); shape (g, ex, xf, Colour (0xff8a8f99), ol * 0.6f);
              if (tier >= 3 && big) { const float f = 0.5f + 0.5f * std::sin (phase * 9.0f); Path fl; fl.startNewSubPath (0.0f, 0.69f); fl.lineTo (-0.06f - 0.04f * f, 0.705f); fl.lineTo (0.0f, 0.725f); fl.closeSubPath(); g.setColour (Colour (0xffff8a3d).withAlpha (0.9f)); g.fillPath (kk::worldsui::placed (fl, xf)); } }
            for (int k = 0; k < 3; ++k) line (0.955f, 0.645f + 0.025f * (float) k, 0.98f, 0.645f + 0.025f * (float) k, Colour (0xff07080c), big ? 2.0f : 1.0f);
            // wheels
            static const uint32 rims[] { 0xff6b3a1c, 0xff9aa0a8, 0xff22d3ee, 0xffd8dde6, 0xffffc83d, 0xffffc83d };
            for (float wx : { 0.25f, 0.79f })
            {
                const float r = 0.13f + 0.02f * t;
                Path ty; ell (ty, wx, 0.77f, r / A, r); shape (g, ty, xf, Colour (0xff1a1b20), ol);
                Path rim; ell (rim, wx, 0.77f, r * 0.6f / A, r * 0.6f); shape (g, rim, xf, Colour (rims[tier]), ol * 0.5f);
                const auto cc = pt (wx, 0.77f);
                g.setColour (Colour (0xff07080c));
                for (int k = 0; k < 5; ++k) { const float a = (float) k / 5.0f * MathConstants<float>::twoPi + (big ? phase * 0.4f : 0.0f), R = r * 0.55f * box.getHeight(); g.drawLine (cc.x, cc.y, cc.x + std::cos (a) * R, cc.y + std::sin (a) * R, big ? 2.0f : 1.0f); }
            }
        }
        else if (tb == kk::worlds::gArsenal)
        {
            if (tier == 0)
            {
                Path wood; thick (wood, 0.16f, 0.52f, 0.60f, 0.50f, 0.055f, A); thick (wood, 0.58f, 0.50f, 0.86f, 0.28f, 0.035f, A); thick (wood, 0.58f, 0.50f, 0.86f, 0.62f, 0.035f, A);
                shape (g, wood, xf, Colour (0xffb07a45), ol);
                line (0.86f, 0.28f, 0.70f, 0.45f, Colour (0xffff5a6e), big ? 4.0f : 1.6f); line (0.70f, 0.45f, 0.86f, 0.62f, Colour (0xffff5a6e), big ? 4.0f : 1.6f);
                Path pouch; ell (pouch, 0.70f, 0.45f, 0.03f, 0.05f); shape (g, pouch, xf, Colour (0xff6b4423), ol * 0.7f);
                for (int k = 0; k < 3; ++k) line (0.42f + 0.035f * (float) k, 0.47f, 0.44f + 0.035f * (float) k, 0.57f, Colour (0xffe8dcc0), big ? 4.0f : 1.6f);
                line (0.30f, 0.54f, 0.30f, 0.64f, Colour (0xffe8dcc0), big ? 2.0f : 1.0f);
                Path bag; bag.startNewSubPath (0.27f, 0.64f); bag.quadraticTo (0.21f, 0.86f, 0.30f, 0.86f); bag.quadraticTo (0.39f, 0.86f, 0.33f, 0.64f); bag.closeSubPath(); shape (g, bag, xf, Colour (0xff8a6a4a), ol);
                Path fe; fe.startNewSubPath (0.50f, 0.47f); fe.quadraticTo (0.44f, 0.24f, 0.56f, 0.12f); fe.quadraticTo (0.58f, 0.32f, 0.52f, 0.47f); fe.closeSubPath(); shape (g, fe, xf, Colour (0xffff8fd0), ol * 0.7f);
                return;
            }
            static const uint32 cols[] { 0, 0xffff8a3d, 0xff3dd6ff, 0xff5bd16b, 0xffb15cff, 0xffffc83d };
            const auto c = Colour (cols[tier]);
            const float L = 0.18f + 0.032f * (float) tier, th = 0.07f + 0.016f * (float) tier;
            if (tier >= 4) glowAt (0.64f + L, 0.42f, 0.18f + 0.05f * (float) (tier - 4), tier == 5 ? Colour (0xfffff2a0) : Colour (0xffd16bff), 0.55f + 0.2f * std::sin (phase * 4.0f));
            // magazine / tank
            if (tier == 2) { Path tank; ell (tank, 0.30f, 0.70f, 0.075f / A * 1.6f, 0.13f); shape (g, tank, xf, Colour (0xff9be7ff).withAlpha (0.8f), ol); }
            else { Path mag = rr (0.255f, 0.55f, 0.09f, 0.30f, 0.02f); shape (g, mag, xf, tier >= 4 ? c.darker (0.3f) : Colour (0xff3a3f4a), ol); if (tier >= 4) glowAt (0.30f, 0.72f, 0.07f, c, 0.6f); }
            // grip + trigger guard
            Path grip; grip.startNewSubPath (0.40f, 0.53f); grip.lineTo (0.53f, 0.53f); grip.lineTo (0.49f, 0.88f); grip.lineTo (0.36f, 0.88f); grip.closeSubPath();
            shape (g, grip.createPathWithRoundedCorners (0.02f), xf, c.darker (0.35f), ol);
            { Path tg; tg.addCentredArc (0.58f, 0.60f, 0.05f, 0.08f, 0.0f, MathConstants<float>::pi * 0.5f, MathConstants<float>::pi * 1.5f, true); g.setColour (Colour (0xff07080c)); g.strokePath (kk::worldsui::placed (tg, xf), PathStrokeType (ol)); }
            { Path tr; thick (tr, 0.565f, 0.55f, 0.55f, 0.65f, 0.012f, A); shape (g, tr, xf, Colour (0xffffd23f), ol * 0.5f); }
            // body
            Path body = tier == 3 ? rr (0.06f, 0.27f, 0.80f, 0.24f, 0.11f) : rr (0.16f, 0.29f, 0.50f, 0.26f, 0.07f);
            shape (g, body, xf, c, ol);
            if (tier == 3) for (int k = 0; k < 3; ++k) { Path st = rr (0.2f + 0.16f * (float) k, 0.27f, 0.05f, 0.24f, 0.0f); shape (g, st, xf, Colour (0xffffd23f), ol * 0.4f); }
            // barrel
            Path bar = rr (0.64f, 0.42f - th * 0.5f, L, th, th * 0.4f);
            shape (g, bar, xf, tier == 5 ? Colour (0xfff4f6f8) : c.darker (0.15f), ol);
            if (tier <= 2) { Path tip = rr (0.62f + L, 0.42f - th * 0.6f, 0.04f, th * 1.2f, 0.01f); shape (g, tip, xf, Colour (0xffff5a1f), ol * 0.8f); }
            if (tier == 1) { Path cork; cork.startNewSubPath (0.66f + L, 0.40f); cork.lineTo (0.72f + L, 0.38f); cork.lineTo (0.72f + L, 0.46f); cork.lineTo (0.66f + L, 0.44f); cork.closeSubPath(); shape (g, cork, xf, Colour (0xffd8b07a), ol * 0.7f); }
            if (tier >= 4) for (int k = 0; k < 3 + tier - 4; ++k) { Path ring; ell (ring, 0.68f + (float) k * L / (float) (3 + tier - 4), 0.42f, 0.012f, th * 0.75f); shape (g, ring, xf, tier == 5 ? Colour (0xffffc83d) : Colour (0xffeaa0ff), ol * 0.6f); }
            if (tier == 2) for (int k = 0; k < 5; ++k) { const float f = std::fmod (phase * 0.25f + (float) k * 0.2f, 1.0f); const auto q = pt (0.70f + L + 0.25f * f, 0.42f - 0.25f * f + 0.04f * std::sin (phase * 2.0f + (float) k)); const float r = (big ? 9.0f : 3.0f) * (0.6f + 0.6f * f); g.setColour (Colour (0xff9be7ff).withAlpha (0.8f * (1.0f - f))); g.drawEllipse (q.x - r, q.y - r, r * 2, r * 2, big ? 1.8f : 1.0f); }
            if (tier == 5) { Path core; ell (core, 0.40f, 0.42f, 0.06f / A * 1.5f, 0.09f); g.setColour (Colour (0xff07080c)); g.fillPath (kk::worldsui::placed (core, xf)); glowAt (0.40f, 0.42f, 0.1f, Colour (0xfffff2a0), 0.9f); }
            // scope
            if (tier >= 2) { Path sc = rr (0.36f, 0.15f, 0.28f, 0.10f, 0.04f); shape (g, sc, xf, Colour (0xff2a2d36), ol); Path ln; ell (ln, 0.635f, 0.20f, 0.012f, 0.045f); shape (g, ln, xf, Colour (0xff9be7ff), ol * 0.5f);
                             Path mt = rr (0.47f, 0.25f, 0.06f, 0.05f, 0.0f); shape (g, mt, xf, Colour (0xff2a2d36), ol * 0.6f); }
            else { Path sc = rr (0.45f, 0.22f, 0.10f, 0.07f, 0.03f); shape (g, sc, xf, c.darker (0.2f), ol); }
        }
        else if (tb == kk::worlds::gHarbour)
        {
            static const uint32 cols[] { 0xff9a6b43, 0xff3d7bd6, 0xfff4f6f8, 0xffff3b5c, 0xfff4f6f8, 0xff12141c };
            const auto c = Colour (cols[tier]);
            const float x0 = 0.1f, x1 = 0.74f + 0.05f * (float) tier;
            // water behind
            { Path w; w.startNewSubPath (-0.02f, 1.0f); for (int k = 0; k <= 30; ++k) { const float x = -0.02f + 1.04f * (float) k / 30.0f; w.lineTo (x, 0.80f + 0.02f * std::sin (x * 22.0f + phase * 2.0f)); } w.lineTo (1.02f, 1.0f); w.closeSubPath();
              g.setGradientFill (ColourGradient (Colour (0xff1f6fd6).withAlpha (0.55f), 0, box.getY() + box.getHeight() * 0.8f, Colour (0xff061830).withAlpha (0.0f), 0, box.getBottom(), false)); g.fillPath (kk::worldsui::placed (w, xf)); }
            // sail (or a flag)
            const bool sail = tier == 2 || tier >= 4;
            line (0.44f, sail ? 0.06f : 0.22f, 0.44f, 0.6f, Colour (0xff3a2a1a), big ? 4.0f : 1.6f);
            if (sail) { Path s; s.startNewSubPath (0.45f, 0.08f); s.quadraticTo (0.62f + 0.02f * std::sin (phase), 0.3f, 0.64f, 0.52f); s.lineTo (0.45f, 0.52f); s.closeSubPath(); shape (g, s, xf, Colour (0xfff8f4ea), ol);
                        Path j; j.startNewSubPath (0.43f, 0.12f); j.lineTo (0.43f, 0.52f); j.lineTo (0.30f, 0.52f); j.closeSubPath(); shape (g, j, xf, Colour (0xffeae4d4), ol); }
            else { Path f; f.startNewSubPath (0.44f, 0.22f); f.quadraticTo (0.50f, 0.20f + 0.02f * std::sin (phase * 4.0f), 0.56f, 0.24f); f.lineTo (0.44f, 0.31f); f.closeSubPath(); shape (g, f, xf, Colour (tier == 3 ? 0xffffd23f : 0xffff3b5c), ol * 0.7f); }
            // superstructure
            if (tier == 1) { Path cab = rr (0.50f, 0.42f, 0.17f, 0.18f, 0.02f); shape (g, cab, xf, Colour (0xfff4f6f8), ol); }
            if (tier == 3) { Path ws; ws.startNewSubPath (0.52f, 0.60f); ws.lineTo (0.60f, 0.46f); ws.lineTo (0.66f, 0.60f); ws.closeSubPath(); shape (g, ws, xf, Colour (0xff9be7ff), ol); }
            if (tier == 4) { Path cab = rr (0.30f, 0.46f, 0.46f, 0.14f, 0.04f); shape (g, cab, xf, Colour (0xffeef2f6), ol); }
            if (tier == 5)
                for (int d = 0; d < 3; ++d)
                {
                    const float dx = 0.06f + 0.1f * (float) d, y = 0.46f - 0.1f * (float) d, w = (x1 - x0) - dx * 2.2f;
                    Path dk = rr (x0 + dx, y, w, 0.11f, 0.03f); shape (g, dk, xf, d == 0 ? Colour (0xfff4f6f8) : Colour (0xffe6eaee), ol);
                    const int nwin = 9 - 2 * d;
                    for (int k = 0; k < nwin; ++k) { const auto q = pt (x0 + dx + 0.03f + (float) k * (w - 0.06f) / (float) (nwin - 1), y + 0.055f); const float ww = big ? 10.0f : 3.0f; g.setColour (Colour (0xff1a2a44)); g.fillRoundedRectangle (q.x - ww * 0.5f, q.y - ww * 0.3f, ww, ww * 0.6f, 2); }
                }
            // engine (outboard)
            { Path en = rr (0.04f, 0.58f, 0.06f, 0.12f, 0.015f); shape (g, en, xf, Colour (0xff2a2d36), ol); Path sh = rr (0.058f, 0.70f, 0.02f, 0.10f, 0.0f); shape (g, sh, xf, Colour (0xff4a4f5a), ol * 0.5f); }
            // hull
            Path hull; hull.startNewSubPath (x0, 0.60f); hull.lineTo (x1, 0.56f - 0.01f * (float) tier); hull.quadraticTo (x1 - 0.02f, 0.80f, x1 - 0.10f, 0.86f); hull.lineTo (x0 + 0.05f, 0.86f); hull.quadraticTo (x0, 0.76f, x0, 0.60f); hull.closeSubPath();
            shape (g, hull, xf, c, ol);
            if (tier >= 4) line (x0 + 0.01f, 0.66f, x1 - 0.03f, 0.63f, Colour (0xffffc83d), big ? 3.5f : 1.4f);
            // deck rail
            for (int k = 0; k < 6; ++k) { const float x = 0.56f + 0.05f * (float) k; if (x > x1 - 0.02f) break; line (x, 0.50f, x, 0.58f, Colour (0xffd8dde6), big ? 2.0f : 1.0f); }
            line (0.55f, 0.50f, jmin (0.82f, x1 - 0.02f), 0.50f, Colour (0xffd8dde6), big ? 2.0f : 1.0f);
            if (tier == 0) for (float ox : { 0.3f, 0.62f }) line (ox, 0.52f, ox + 0.1f, 0.9f, Colour (0xffc9a27a), big ? 5.0f : 2.0f);
            // foam in front
            for (int k = 0; k < 8; ++k) { const auto q = pt (x0 + (x1 - x0) * (float) k / 7.0f, 0.87f + 0.01f * std::sin (phase * 3.0f + (float) k)); g.setColour (Colours::white.withAlpha (0.55f)); g.fillEllipse (q.x - (big ? 10.0f : 4.0f), q.y - 2, big ? 20.0f : 8.0f, big ? 6.0f : 3.0f); }
        }
        else
        {
            static const uint32 cols[] { 0xffd9c7a0, 0xffffd23f, 0xffe6eef5, 0xfff4f6f8, 0xff1b1d26, 0xfff4f6f8 };
            static const uint32 trims[] { 0xff8a5a3a, 0xff1a1b20, 0xff3d7bd6, 0xff3d7bd6, 0xffffc83d, 0xffffc83d };
            const auto c = Colour (cols[tier]), tr = Colour (trims[tier]);
            const bool jet = tier >= 3;
            if (jet && big) for (int k = 0; k < 2; ++k) { const auto a = pt (0.2f, 0.33f + 0.02f * (float) k), b = pt (-0.4f, 0.30f + 0.03f * (float) k); g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.3f), a.x, a.y, Colours::white.withAlpha (0.0f), b.x, b.y, false)); g.drawLine (a.x, a.y, b.x, b.y, 6.0f); }
            // tail fin (+ the upper wing of the old biplane)
            Path fin; fin.startNewSubPath (0.07f, 0.40f); fin.lineTo (0.03f, 0.10f); fin.lineTo (0.12f, 0.10f); fin.lineTo (0.24f, 0.39f); fin.closeSubPath(); shape (g, fin, xf, tier >= 4 ? tr : c, ol);
            if (tier == 1) { Path up = rr (0.42f, 0.22f, 0.22f, 0.04f, 0.015f); shape (g, up, xf, c, ol); for (float sx : { 0.46f, 0.6f }) line (sx, 0.26f, sx, 0.38f, Colour (0xff07080c), big ? 2.5f : 1.0f); }
            // fuselage
            Path fu; fu.startNewSubPath (0.06f, 0.36f); fu.lineTo (0.20f, 0.37f); fu.lineTo (0.84f, 0.36f); fu.cubicTo (0.93f, 0.36f, 0.99f, 0.43f, 0.985f, 0.47f); fu.cubicTo (0.98f, 0.52f, 0.92f, 0.55f, 0.84f, 0.55f); fu.lineTo (0.20f, 0.54f); fu.lineTo (0.08f, 0.47f); fu.closeSubPath();
            shape (g, fu, xf, c, ol);
            line (0.1f, 0.50f, 0.9f, 0.51f, tr, big ? 3.5f : 1.4f);
            // windows
            const int nw = 2 + 2 * tier;
            for (int row = 0; row < (tier == 5 ? 2 : 1); ++row)
                for (int k = 0; k < nw; ++k) { const auto q = pt (0.30f + 0.5f * (float) k / (float) jmax (1, nw - 1), 0.43f - 0.045f * (float) row); const float w = big ? 9.0f : 3.0f; g.setColour (Colour (0xff1a2a44)); g.fillRoundedRectangle (q.x - w * 0.5f, q.y - w * 0.6f, w, w * 1.2f, w * 0.4f); }
            { Path ck; ck.startNewSubPath (0.86f, 0.40f); ck.lineTo (0.93f, 0.40f); ck.lineTo (0.96f, 0.45f); ck.lineTo (0.87f, 0.45f); ck.closeSubPath(); shape (g, ck, xf, Colour (0xff9be7ff), ol * 0.6f); }
            // wing
            Path wing; if (jet) { wing.startNewSubPath (0.46f, 0.51f); wing.lineTo (0.64f, 0.51f); wing.lineTo (0.50f, 0.88f); wing.lineTo (0.40f, 0.88f); }
                       else { wing.startNewSubPath (0.47f, 0.51f); wing.lineTo (0.62f, 0.51f); wing.lineTo (0.56f, 0.86f); wing.lineTo (0.44f, 0.86f); }
            wing.closeSubPath(); shape (g, wing, xf, c.darker (0.12f), ol);
            if (jet) { Path wl; wl.startNewSubPath (0.40f, 0.88f); wl.lineTo (0.50f, 0.88f); wl.lineTo (0.47f, 0.94f); wl.closeSubPath(); shape (g, wl, xf, tr, ol * 0.7f); }
            // engines
            auto prop = [&] (float x, float y, float h)
            {
                Path hub; ell (hub, x, y, 0.012f, 0.03f); shape (g, hub, xf, Colour (0xff3a3f4a), ol * 0.6f);
                const float sw = std::sin (phase * 11.0f);
                Path bl; ell (bl, x + 0.004f, y, 0.008f, h * (0.6f + 0.4f * std::abs (sw))); g.setColour (Colour (0xff07080c).withAlpha (0.55f)); g.fillPath (kk::worldsui::placed (bl, xf));
                Path blur; ell (blur, x + 0.004f, y, 0.006f, h); g.setColour (Colours::white.withAlpha (0.12f)); g.fillPath (kk::worldsui::placed (blur, xf));
            };
            if (! jet) { prop (0.985f, 0.47f, 0.14f); if (tier == 2) { Path na = rr (0.48f, 0.60f, 0.11f, 0.06f, 0.02f); shape (g, na, xf, c.darker (0.2f), ol); prop (0.595f, 0.63f, 0.09f); } }
            else
            {
                const float r = 0.05f + 0.008f * (float) (tier - 3);
                Path py; thick (py, 0.30f, 0.37f, 0.31f, 0.33f, 0.01f, A); shape (g, py, xf, c.darker (0.2f), ol * 0.6f);
                Path pod = rr (0.20f, 0.33f - r, 0.17f, r * 2, r); shape (g, pod, xf, tier >= 4 ? tr : Colour (0xffc8ced6), ol);
                Path in; ell (in, 0.37f, 0.33f, 0.012f, r * 0.85f); shape (g, in, xf, Colour (0xff2a2d36), ol * 0.5f);
                glowAt (0.19f, 0.33f, 0.07f, Colour (0xffff8a3d), 0.5f + 0.3f * std::sin (phase * 8.0f));
            }
        }
    }
    void drawStage (Graphics& g)
    {
        const auto s = stage().toFloat();
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (s, 18); g.reduceClipRegion (clip);
        g.setGradientFill (ColourGradient (Colour (0xff17130c), s.getX(), s.getY(), Colour (0xff050506), s.getX(), s.getBottom(), false)); g.fillRect (s);
        const auto gold = Colour (0xffffc83d);
        // the ladder: worst ... most expensive
        const auto L = ladder();
        {
            const float y = L.getBottom() + 12;
            g.setGradientFill (ColourGradient (Colour (0xff6b5a3a), L.getX(), y, gold, L.getRight(), y, false)); g.fillRect (L.getX(), y - 1.0f, L.getWidth() - 10, 2.0f);
            Path ar; ar.addTriangle (L.getRight() - 12, y - 6, L.getRight(), y, L.getRight() - 12, y + 6); g.setColour (gold); g.fillPath (ar);
            g.setFont (kk::modern::font (10.5f, true, 0.3f)); g.setColour (Colours::white.withAlpha (0.45f)); g.drawText ("WORST", Rectangle<float> (L.getX(), y + 2, 120, 14), Justification::centredLeft);
            g.setColour (gold.withAlpha (0.85f)); g.drawText ("MOST EXPENSIVE", Rectangle<float> (L.getRight() - 200, y + 2, 186, 14), Justification::centredRight);
        }
        const int tier = tiers[(size_t) tab];
        for (int i = 0; i < 6; ++i)
        {
            const auto r = rung (i).translated (0, i == tier ? -3.0f * rungFlash : 0.0f);
            const bool on = i == tier, hov = i == hoverRung;
            const auto rc = Colour (kk::worlds::rarityColour (i));
            g.setGradientFill (ColourGradient (on ? Colour (0xff2a2210) : Colour (0xff121216), r.getX(), r.getY(), Colour (0xff08080a), r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 12);
            drawItem (g, tab, i, r.reduced (16, 0).withTrimmedTop (20).withHeight (r.getHeight() * 0.46f), false);
            g.setColour (rc.withAlpha (0.9f)); g.setFont (kk::modern::font (9.5f, true, 0.3f)); g.drawText (kk::worlds::rarity (i), r.reduced (10, 4).withHeight (14), Justification::centredLeft);
            g.setColour (Colours::white.withAlpha (on ? 1.0f : 0.8f)); g.setFont (kk::modern::font (11.5f, true, 0.05f));
            g.drawFittedText (kk::worlds::items (tab)[(size_t) i].name, r.withTrimmedTop (r.getHeight() * 0.64f).withHeight (18).reduced (6, 0).toNearestInt(), Justification::centred, 1, 0.7f);
            const String pr (kk::worlds::items (tab)[(size_t) i].price);
            g.setFont (kk::modern::font (11.0f, true, 0.12f));
            const float pw = GlyphArrangement::getStringWidth (g.getCurrentFont(), pr) + 20;
            const Rectangle<float> pill (r.getCentreX() - pw * 0.5f, r.getBottom() - 23, pw, 19);
            g.setColour (gold.withAlpha (on ? 0.95f : 0.18f)); g.fillRoundedRectangle (pill, 9.5f);
            g.setColour (on ? Colour (0xff1a1206) : gold); g.drawText (pr, pill, Justification::centred);
            if (on) { Path b; b.addRoundedRectangle (r, 12); kk::worldsui::glowPath (g, b, gold, 0.7f + rungFlash); g.setColour (gold); g.strokePath (b, PathStrokeType (2.0f)); }
            else { g.setColour (Colours::white.withAlpha (hov ? 0.4f : 0.12f)); g.drawRoundedRectangle (r, 12, 1.0f); }
        }
        // the showroom: a spotlight, a glossy floor
        const auto sr = showroom();
        g.setGradientFill (ColourGradient (gold.withAlpha (0.14f), sr.getCentreX(), sr.getY(), gold.withAlpha (0.0f), sr.getCentreX(), sr.getBottom(), false));
        Path cone; cone.startNewSubPath (sr.getCentreX() - 60, sr.getY()); cone.lineTo (sr.getCentreX() + 60, sr.getY()); cone.lineTo (sr.getCentreX() + sr.getWidth() * 0.42f, sr.getBottom()); cone.lineTo (sr.getCentreX() - sr.getWidth() * 0.42f, sr.getBottom()); cone.closeSubPath();
        g.fillPath (cone);
        const auto ib = itemBox();
        g.setGradientFill (ColourGradient (gold.withAlpha (0.2f), ib.getCentreX(), ib.getBottom() - ib.getHeight() * 0.05f, gold.withAlpha (0.0f), ib.getCentreX() + ib.getWidth() * 0.6f, ib.getBottom(), true));
        g.fillEllipse (ib.getX() - 40, ib.getBottom() - ib.getHeight() * 0.18f, ib.getWidth() + 80, ib.getHeight() * 0.3f);
        for (int k = 0; k < 7; ++k) { const float y = ib.getBottom() + 8 + (float) k * (float) k * 3.0f; if (y > sr.getBottom()) break; g.setColour (gold.withAlpha (0.06f)); g.drawHorizontalLine ((int) y, sr.getX(), sr.getRight()); }
        drawItem (g, tab, tier, ib, true);
        // its name and story
        const auto& it = kk::worlds::items (tab)[(size_t) tier];
        g.setColour (gold); g.setFont (kk::modern::font (24.0f, true, 0.12f)); g.drawText (it.name, Rectangle<float> (sr.getX() + 6, sr.getY() + 2, 520, 30), Justification::centredLeft);
        g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (12.0f, true, 0.04f)); g.drawText (it.hint, Rectangle<float> (sr.getX() + 6, sr.getY() + 32, 520, 16), Justification::centredLeft);
        // the parts
        const auto ms = markers();
        for (int i = 0; i < (int) kk::worlds::garageParts (tab).size(); ++i)
        {
            const auto P = kk::worlds::garagePart (tab, i, tier);
            const auto c = partCentre (i); const auto m = ms[(size_t) i];
            const bool hov = hoverPart == i; const float fl = flashPart == i ? flash : 0.0f;
            const float pr = 10.0f + 3.0f * std::sin (phase * 3.0f + (float) i) + 16.0f * fl;
            const Point<float> mp (m.getCentreX(), m.getCentreY() > c.y ? m.getY() : m.getBottom());
            g.setColour (gold.withAlpha (0.5f)); g.drawLine (c.x, c.y, mp.x, mp.y, 1.0f);
            g.setColour (gold.withAlpha (hov ? 0.95f : 0.6f)); g.drawEllipse (c.x - pr, c.y - pr, pr * 2, pr * 2, hov ? 2.4f : 1.6f);
            g.fillEllipse (c.x - 3.5f, c.y - 3.5f, 7, 7);
            const bool hot = hov || fl > 0.2f;
            g.setColour (hot ? gold : Colour (0xee0c0b08)); g.fillRoundedRectangle (m, 13);
            g.setColour (gold); g.drawRoundedRectangle (m, 13, 1.3f);
            g.setColour (hot ? Colour (0xff1a1206) : Colours::white); g.setFont (kk::modern::font (11.5f, true, 0.22f));
            g.drawText (P.name, m, Justification::centred);
        }
        g.setColour (Colours::white.withAlpha (0.4f)); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText ("click a part = the chosen sound is re-shaped that way (again = further)", sr.withTrimmedTop (sr.getHeight() - 16).reduced (6, 0), Justification::centredLeft);
    }
    void drawInfo (Graphics& g)
    {
        const auto a = panelTop().toFloat();
        const int tier = tiers[(size_t) tab];
        const auto& it = kk::worlds::items (tab)[(size_t) tier];
        const bool night = kk::theme().night;
        const auto gold = night ? Colour (0xffffc83d) : Colour (0xff8a5a00), rc = Colour (kk::worlds::rarityColour (tier)).withMultipliedBrightness (night ? 1.0f : 0.6f);
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.5f, true, 0.3f)); g.drawText (kk::worlds::garageName (tab), a.withHeight (16), Justification::centredLeft);
        g.setColour (kk::theme().text); g.setFont (kk::modern::font (21.0f, true, 0.06f)); g.drawFittedText (it.name, a.withTrimmedTop (18).withHeight (28).toNearestInt(), Justification::centredLeft, 1, 0.7f);
        float y = a.getY() + 52;
        g.setFont (kk::modern::font (11.5f, true, 0.25f));
        const String rar (kk::worlds::rarity (tier)); const float rw = GlyphArrangement::getStringWidth (g.getCurrentFont(), rar) + 22;
        g.setColour (rc.withAlpha (0.2f)); g.fillRoundedRectangle (a.getX(), y, rw, 22, 11); g.setColour (rc); g.drawRoundedRectangle (a.getX(), y, rw, 22, 11, 1.3f); g.drawText (rar, Rectangle<float> (a.getX(), y, rw, 22), Justification::centred);
        const String pr (it.price); const float pw = GlyphArrangement::getStringWidth (g.getCurrentFont(), pr) + 22;
        g.setColour (Colour (0xffffc83d)); g.fillRoundedRectangle (a.getX() + rw + 8, y, pw, 22, 11); g.setColour (Colour (0xff1a1206)); g.drawText (pr, Rectangle<float> (a.getX() + rw + 8, y, pw, 22), Justification::centred);
        y += 36;
        g.setColour (kk::theme().text.withAlpha (0.85f)); g.setFont (kk::modern::font (11.5f, true, 0.3f)); g.drawText ("IN ITS CHARACTER", Rectangle<float> (a.getX(), y, a.getWidth(), 16), Justification::centredLeft); y += 20;
        String fl; for (auto& f : kk::worlds::flavours (tab)) fl << (fl.isEmpty() ? "" : "  /  ") << f.word;
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (12.5f, true, 0.05f)); g.drawText (fl, Rectangle<float> (a.getX(), y, a.getWidth(), 18), Justification::centredLeft); y += 22;
        g.drawFittedText (tier >= 4 ? "top shelf: big, long, layered recipes" : tier >= 2 ? "rarer: wider, longer, with a second layer" : "the cheap end: dry, thin and dirty", Rectangle<float> (a.getX(), y, a.getWidth(), 18).toNearestInt(), Justification::centredLeft, 1, 0.8f);
        y += 30;
        g.setColour (kk::theme().text.withAlpha (0.85f)); g.setFont (kk::modern::font (11.5f, true, 0.3f)); g.drawText ("RE-SHAPED BY", Rectangle<float> (a.getX(), y, a.getWidth(), 16), Justification::centredLeft); y += 20;
        g.setFont (kk::modern::font (12.5f, true, 0.12f));
        if (touched.isEmpty()) { g.setColour (kk::theme().dim); g.drawText ("click a part of the item", Rectangle<float> (a.getX(), y, a.getWidth(), 18), Justification::centredLeft); }
        else
        {
            float x = a.getX();
            for (auto& s : touched)
            {
                const float w = GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 20;
                if (x + w > a.getRight()) { x = a.getX(); y += 26; }
                g.setColour (gold.withAlpha (0.18f)); g.fillRoundedRectangle (x, y, w, 22, 11); g.setColour (gold); g.drawText (s, Rectangle<float> (x, y, w, 22), Justification::centred);
                x += w + 6;
            }
        }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        tickBase();
        if (flash > 0) flash = jmax (0.0f, flash - 0.03f);
        if (rungFlash > 0) rungFlash = jmax (0.0f, rungFlash - 0.06f);
        repaint (stage()); repaint (strip());
    }

    HotButton tabA { lnf }, tabB { lnf }, tabC { lnf }, tabD { lnf };
    std::array<HotButton*, 4> tabs { &tabA, &tabB, &tabC, &tabD };
    int tab = 1, hoverRung = -1, hoverPart = -1, flashPart = -1;
    std::array<int, 4> tiers { 2, 2, 2, 2 };
    StringArray touched;
    float flash = 0, rungFlash = 0;
};

//==============================================================================
// PARTY: a club. Drag the crowd up (more people dance), brighten the lights, spin the disco ball faster (circles on it) = ENERGY.
// The page makes dance sounds (house chords, disco strings, organ stabs, plucks, basslines) and plays a 4-on-the-floor phrase:
// more energy = brighter, busier, euphoric; calm = deep, warm, sparse. Click the DJ booth = the groove on / off.
class PartyPage : public WorldPage
{
public:
    PartyPage (KeysKillaProcessor& p, KKLookAndFeel& l) : WorldPage (p, l, Colour (0xffff4fd8))
    {
        grooveBtn.framed = true; grooveBtn.setTooltip ("The 4-on-the-floor phrase on / off (or click the DJ booth)");
        grooveBtn.onClick = [this] { setGroove (! groove); };
        addAndMakeVisible (grooveBtn);
        seed = 5; showDna = false;
        refreshGroove();
        reroll (false);
        setMouseCursor (MouseCursor::PointingHandCursor);
    }
    ~PartyPage() override { if (grooveMine && proc.loopOwnerId() == 4) proc.stopLoop(); }
    // debug / snapshot: 84 = the party at high energy
    void debugShow (int) { party.crowd = 0.92f; party.lights = 0.9f; party.spin = 0.95f; ballAngle = 1.3f; phase = 2.4f; beat = 6.2f; reroll (false); }
    void setEnergy (float crowd, float lights, float spin) { party.crowd = crowd; party.lights = lights; party.spin = spin; live(); }
    float energy() const { return party.energy(); }
    void visibilityChanged() override
    {
        if (! isVisible() && grooveMine && proc.loopOwnerId() == 4) { proc.stopLoop(); grooveMine = false; }
        WorldPage::visibilityChanged();
    }
    void paint (Graphics& g) override
    {
        pageBackdrop (g, *this);
        drawHeader (g, "PARTY", "drag the crowd UP = more people dance.   drag the lights UP = brighter.   spin the disco ball in circles = faster.   more energy = brighter, busier, euphoric");
        drawClub (g);
        drawPanel (g);
        drawInfo (g);
        drawStrip (g);
    }
    void layoutTabs() override { const auto a = panelTop(); grooveBtn.setBounds (a.getX(), a.getBottom() - 40, a.getWidth(), 40); }
    void mouseMove (const MouseEvent& e) override { stripHover (e.position); }
    void mouseDown (const MouseEvent& e) override
    {
        if (stripDown (e)) return;
        if (! stage().contains (e.getPosition())) return;
        lastMs = Time::getMillisecondCounter();
        if (e.position.getDistanceFrom (ballCentre()) < ballR() * 1.8f) { mode = 3; lastAng = std::atan2 (e.position.y - ballCentre().y, e.position.x - ballCentre().x); angVel = 0; return; }
        if (booth().contains (e.position)) { setGroove (! groove); return; }
        if (e.position.y < booth().getY()) { mode = 2; start = party.lights; }
        else { mode = 1; start = party.crowd; }
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (mode == 0) return;
        const auto d = e.position - e.mouseDownPosition;
        const auto now = Time::getMillisecondCounter();
        if (mode == 1) party.crowd = jlimit (0.0f, 1.0f, start - d.y / 220.0f + d.x / 900.0f);
        else if (mode == 2) party.lights = jlimit (0.0f, 1.0f, start - d.y / 180.0f + d.x / 600.0f);
        else
        {
            const auto c = ballCentre();
            const float a = std::atan2 (e.position.y - c.y, e.position.x - c.x);
            float da = a - lastAng;
            while (da > MathConstants<float>::pi) da -= MathConstants<float>::twoPi;
            while (da < -MathConstants<float>::pi) da += MathConstants<float>::twoPi;
            const float dt = jmax (0.005f, (float) (now - lastMs) / 1000.0f);
            angVel = angVel * 0.7f + 0.3f * std::abs (da) / dt;
            party.spin = jlimit (0.0f, 1.0f, party.spin + (jlimit (0.0f, 1.0f, angVel / 14.0f) - party.spin) * 0.25f);
            ballAngle += da; lastAng = a;
        }
        lastMs = now;
        if (now - lastLive > 320) { lastLive = now; live(); }
        repaint();
    }
    void mouseUp (const MouseEvent&) override { if (mode != 0) { mode = 0; live(); note = "ENERGY: " + kk::worlds::energyWord (party.energy()); } }
private:
    void reroll (bool audition) override { setRoll (kk::worlds::partyRoll (party.energy(), seed), audition, jmax (0, sel)); }
    void live() { setRoll (kk::worlds::partyRoll (party.energy(), seed), false, jmax (0, sel)); }
    void heard() override
    {
        if (! groove || ! isPositiveAndBelow (sel, (int) roll.size())) return;
        std::vector<kk::LoopNote> ln;
        for (auto& n : kk::worlds::partyPhrase (party.energy(), seed, roll[(size_t) sel].kind)) ln.push_back ({ n.start, n.len, n.note, n.low });
        proc.playCustomLoop (ln, 16.0);
        grooveMine = true;
    }
    void setGroove (bool on)
    {
        groove = on; refreshGroove();
        if (! on) { if (proc.loopOwnerId() == 4) proc.stopLoop(); grooveMine = false; }
        else hear (false);
        repaint();
    }
    void refreshGroove() { grooveBtn.setButtonText (groove ? "STOP THE GROOVE" : "PLAY THE GROOVE"); grooveBtn.selected = groove; grooveBtn.repaint(); }
    Point<float> ballCentre() const { const auto s = stage().toFloat(); return { s.getCentreX(), s.getY() + 118 }; }
    float ballR() const { return 46.0f; }
    Rectangle<float> booth() const { const auto s = stage().toFloat(); return { s.getCentreX() - 190, s.getBottom() - 262, 380, 100 }; }
    void drawPerson (Graphics& g, float x, float y, float sc, bool dancing, int i, Colour lit, float e)
    {
        const float bounce = dancing ? std::abs (std::sin ((beat + (float) (i % 3) * 0.08f) * MathConstants<float>::pi)) * (4.0f + 10.0f * e) * sc : 0.0f;
        const float yy = y - bounce;
        const auto c = dancing ? Colour (0xff1a1028).interpolatedWith (lit, 0.35f + 0.3f * party.lights) : Colour (0xff17141d);
        Path p;
        p.addEllipse (x - 9 * sc, yy - 54 * sc, 18 * sc, 20 * sc);
        p.addRoundedRectangle (x - 14 * sc, yy - 34 * sc, 28 * sc, 44 * sc, 9 * sc);
        g.setColour (c); g.fillPath (p);
        const float armUp = dancing ? (i % 2 == 0 ? 1.0f : std::abs (std::sin (beat * MathConstants<float>::halfPi + (float) i))) * (0.4f + 0.6f * e) : 0.0f;
        for (int sd = -1; sd <= 1; sd += 2)
        {
            const float sx = x + (float) sd * 12 * sc, sy = yy - 28 * sc;
            const float hx = sx + (float) sd * (10 + 6 * armUp) * sc, hy = sy + (24 - 58 * armUp) * sc;
            g.drawLine (sx, sy, hx, hy, 6.0f * sc);
            g.fillEllipse (hx - 4 * sc, hy - 4 * sc, 8 * sc, 8 * sc);
        }
        if (dancing) { g.setColour (lit.withAlpha (0.25f + 0.5f * party.lights)); g.drawEllipse (x - 9 * sc, yy - 54 * sc, 18 * sc, 20 * sc, 1.4f); }
    }
    void drawClub (Graphics& g)
    {
        const auto s = stage().toFloat();
        const float e = party.energy();
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (s, 18); g.reduceClipRegion (clip);
        g.setGradientFill (ColourGradient (Colour (0xff120a22).interpolatedWith (Colour (0xff2a0c3a), e), s.getX(), s.getY(), Colour (0xff040308), s.getX(), s.getBottom(), false)); g.fillRect (s);
        static const uint32 lc[] { 0xffff4fd8, 0xff22d3ee, 0xffffc83d, 0xffb15cff, 0xff36ff9a, 0xffff5a6e, 0xff4d9dff, 0xffff8a3d };
        // the dance floor
        const float fy = booth().getBottom() - 10;
        for (int k = 0; k < 9; ++k)
        {
            const float y = fy + (s.getBottom() - fy) * std::pow ((float) k / 8.0f, 1.6f);
            g.setColour (Colour (lc[(size_t) ((k + (int) beat) % 8)]).withAlpha (0.05f + 0.14f * party.lights * (k % 2 ? 1.0f : 0.5f))); g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
        }
        for (int k = -8; k <= 8; ++k) { const float x0 = s.getCentreX() + (float) k * 40.0f, x1 = s.getCentreX() + (float) k * 150.0f; g.setColour (Colours::white.withAlpha (0.04f + 0.04f * party.lights)); g.drawLine (x0, fy, x1, s.getBottom(), 1.0f); }
        // light cones from the truss
        const int nl = 8; const float ty = s.getY() + 22;
        for (int i = 0; i < nl; ++i)
        {
            const float x = s.getX() + 70 + (s.getWidth() - 140) * (float) i / (float) (nl - 1);
            const float ang = std::sin (beat * (0.25f + 0.2f * e) + (float) i * 1.3f) * (0.25f + 0.45f * e);
            const float len = s.getHeight() * 0.95f, w = 50.0f + 40.0f * party.lights;
            const auto c = Colour (lc[(size_t) i]);
            const Point<float> tip (x + std::sin (ang) * len, ty + std::cos (ang) * len);
            const Point<float> nrm (std::cos (ang), -std::sin (ang));
            Path cone; cone.startNewSubPath (x, ty + 8); cone.lineTo (tip + nrm * w); cone.lineTo (tip - nrm * w); cone.closeSubPath();
            const float strobe = e > 0.8f && std::fmod (beat, 1.0f) < 0.12f ? 0.15f : 0.0f;
            g.setGradientFill (ColourGradient (c.withAlpha (0.05f + 0.28f * party.lights + strobe), x, ty, c.withAlpha (0.0f), tip.x, tip.y, false)); g.fillPath (cone);
            g.setColour (c.withAlpha (0.1f + 0.4f * party.lights)); g.fillEllipse (tip.x - w, tip.y - 10, w * 2, 20);
        }
        // truss + lamps
        g.setColour (Colour (0xff2a2a33)); g.fillRect (s.getX() + 30, ty - 8, s.getWidth() - 60, 10.0f);
        g.setColour (Colour (0xff4a4a58)); for (float x = s.getX() + 30; x < s.getRight() - 30; x += 18) g.drawLine (x, ty - 8, x + 9, ty + 2, 1.0f);
        for (int i = 0; i < nl; ++i) { const float x = s.getX() + 70 + (s.getWidth() - 140) * (float) i / (float) (nl - 1); g.setColour (Colour (0xff1a1a22)); g.fillRoundedRectangle (x - 10, ty, 20, 16, 4); g.setColour (Colour (lc[(size_t) i]).withAlpha (0.4f + 0.6f * party.lights)); g.fillEllipse (x - 6, ty + 8, 12, 10); }
        // disco ball: facets turning with its spin, sparkles all over the room
        const auto bc = ballCentre(); const float br = ballR();
        g.setColour (Colour (0xff8a8f99)); g.drawLine (bc.x, ty, bc.x, bc.y - br, 1.5f);
        Random rr (17);
        const int nsp = 14 + (int) (60.0f * party.spin * (0.4f + 0.6f * party.lights));
        for (int k = 0; k < nsp; ++k)
        {
            const float a = rr.nextFloat() * MathConstants<float>::twoPi + ballAngle * (0.6f + 0.4f * rr.nextFloat()), R = 80.0f + rr.nextFloat() * s.getWidth() * 0.55f;
            const float x = bc.x + std::cos (a) * R, y = bc.y + 30 + std::abs (std::sin (a)) * R * 0.75f;
            const float r = 1.5f + 2.5f * rr.nextFloat();
            g.setColour (Colour (lc[(size_t) (k % 8)]).brighter (0.5f).withAlpha (0.3f + 0.5f * party.spin)); g.fillEllipse (x - r, y - r, r * 2, r * 2);
        }
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.12f + 0.25f * party.spin), bc.x, bc.y, Colours::white.withAlpha (0.0f), bc.x + br * 2.6f, bc.y, true));
        g.fillEllipse (bc.x - br * 2.6f, bc.y - br * 2.6f, br * 5.2f, br * 5.2f);
        {
            Graphics::ScopedSaveState bs (g);
            Path ball; ball.addEllipse (bc.x - br, bc.y - br, br * 2, br * 2);
            g.setGradientFill (ColourGradient (Colour (0xffd8dde6), bc.x - br * 0.4f, bc.y - br * 0.5f, Colour (0xff2a2d36), bc.x + br, bc.y + br, true)); g.fillPath (ball);
            g.reduceClipRegion (ball);
            const int rows = 9, cols = 16;
            for (int rI = 0; rI < rows; ++rI)
            {
                const float lat = -MathConstants<float>::halfPi + MathConstants<float>::pi * ((float) rI + 0.5f) / (float) rows, cl = std::cos (lat);
                const float y = bc.y + std::sin (lat) * br, h = br * MathConstants<float>::pi / (float) rows * cl * 0.9f + 1.0f;
                for (int cI = 0; cI < cols; ++cI)
                {
                    const float lon = MathConstants<float>::twoPi * (float) cI / (float) cols + ballAngle;
                    if (std::cos (lon) <= 0.05f) continue;
                    const float x = bc.x + std::sin (lon) * br * cl, w = br * MathConstants<float>::twoPi / (float) cols * std::cos (lon) * cl * 0.85f;
                    const float glint = 0.5f + 0.5f * std::sin ((float) (rI * 7 + cI * 13) + ballAngle * 3.0f);
                    g.setColour (Colours::white.withAlpha (0.12f + 0.55f * glint * std::cos (lon))); g.fillRect (x - w * 0.5f, y - h * 0.5f, w, h);
                }
            }
        }
        g.setColour (Colours::white.withAlpha (0.25f + 0.5f * party.spin)); g.drawEllipse (bc.x - br, bc.y - br, br * 2, br * 2, 1.4f);
        // the DJ
        const auto bo = booth();
        {
            const float dj = std::abs (std::sin (beat * MathConstants<float>::pi)) * (3.0f + 6.0f * e);
            const float hx = bo.getCentreX(), hy = bo.getY() - 34 - dj;
            const float up = e > 0.6f ? std::abs (std::sin (beat * MathConstants<float>::halfPi)) : 0.0f;
            Path djp; djp.addRoundedRectangle (hx - 34, hy + 20, 68, 64, 16); djp.addEllipse (hx - 19, hy - 18, 38, 40);
            Path arms; arms.startNewSubPath (hx - 28, hy + 30); arms.lineTo (hx - 64, hy + 44 - 76 * up); arms.startNewSubPath (hx + 28, hy + 30); arms.lineTo (hx + 56, hy + 58);
            g.setColour (Colour (0xff3a2a52)); g.strokePath (arms, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::rounded)); g.fillPath (djp);
            g.setColour (Colour (0xffff4fd8).withAlpha (0.55f)); g.strokePath (djp, PathStrokeType (1.6f));
            g.setColour (Colour (0xff22d3ee).withAlpha (0.35f)); g.strokePath (arms, PathStrokeType (1.4f));
            g.setColour (Colour (0xffff4fd8)); g.drawEllipse (hx - 22, hy - 12, 44, 30, 3.0f);
            g.fillRoundedRectangle (hx - 27, hy - 2, 9, 15, 3); g.fillRoundedRectangle (hx + 18, hy - 2, 9, 15, 3);
        }
        Path desk; desk.startNewSubPath (bo.getX(), bo.getY()); desk.lineTo (bo.getRight(), bo.getY()); desk.lineTo (bo.getRight() - 20, bo.getBottom()); desk.lineTo (bo.getX() + 20, bo.getBottom()); desk.closeSubPath();
        g.setGradientFill (ColourGradient (Colour (0xff2a2236), bo.getX(), bo.getY(), Colour (0xff0d0a14), bo.getX(), bo.getBottom(), false)); g.fillPath (desk);
        g.setColour (Colour (0xffff4fd8).withAlpha (groove ? 0.9f : 0.3f)); g.strokePath (desk, PathStrokeType (1.6f));
        for (int k = 0; k < 2; ++k)
        {
            const Point<float> tc (bo.getX() + 80 + (float) k * (bo.getWidth() - 160), bo.getY() + 24);
            g.setColour (Colour (0xff0a0a10)); g.fillEllipse (tc.x - 34, tc.y - 13, 68, 26);
            g.setColour (Colours::white.withAlpha (0.35f)); const float a = beat * MathConstants<float>::pi * (groove ? 1.0f : 0.0f); g.drawLine (tc.x, tc.y, tc.x + std::cos (a) * 30, tc.y + std::sin (a) * 11, 1.4f);
        }
        const int nled = 24;
        for (int k = 0; k < nled; ++k)
        {
            const float lv = groove ? std::abs (std::sin (beat * 2.0f + (float) k * 0.4f)) * (0.3f + 0.7f * e) : 0.1f;
            g.setColour (Colour (lc[(size_t) (k % 8)]).withAlpha (0.2f + 0.8f * lv)); g.fillRoundedRectangle (bo.getX() + 34 + (float) k * (bo.getWidth() - 68) / (float) nled, bo.getBottom() - 46, (bo.getWidth() - 68) / (float) nled - 3, 9, 2);
        }
        g.setColour (Colours::white.withAlpha (0.5f)); g.setFont (kk::modern::font (10.5f, true, 0.25f));
        g.drawText (groove ? "DJ BOOTH - click = stop the groove" : "DJ BOOTH - click = play the groove", bo.withTrimmedTop (bo.getHeight() - 30).withTrimmedBottom (10), Justification::centred);
        // the crowd: three rows; the more you drag it up, the more of them dance
        constexpr int N = 33; Random cr (3);
        std::array<int, N> order {}; for (int i = 0; i < N; ++i) order[(size_t) i] = i;
        for (int i = N - 1; i > 0; --i) std::swap (order[(size_t) i], order[(size_t) cr.nextInt (i + 1)]);
        std::array<bool, N> dancing {}; const int nd = (int) std::round (party.crowd * (float) N);
        for (int i = 0; i < nd; ++i) dancing[(size_t) order[(size_t) i]] = true;
        for (int row = 0; row < 3; ++row)
            for (int k = 0; k < 11; ++k)
            {
                const int i = row * 11 + k;
                const float sc = 0.85f + 0.3f * (float) row;
                const float x = s.getX() + 40 + (s.getWidth() - 80) * ((float) k + (row % 2 ? 0.5f : 0.0f)) / 11.0f;
                const float y = s.getBottom() - 96 + (float) row * 36.0f;
                drawPerson (g, x, y, sc, dancing[(size_t) i], i, Colour (lc[(size_t) ((k + row) % 8)]), e);
            }
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText ("drag up on the crowd or the lights,  draw circles on the disco ball", Rectangle<float> (s.getX() + 16, s.getY() + 40, 520, 16), Justification::centredLeft);
    }
    void drawInfo (Graphics& g)
    {
        const auto a = panelTop().toFloat();
        const float e = party.energy();
        auto energyCol = [] (float f) { return Colour (0xff22d3ee).interpolatedWith (Colour (0xffff4fd8), jmin (1.0f, f * 1.4f)).interpolatedWith (Colour (0xffffc83d), jlimit (0.0f, 1.0f, (f - 0.6f) * 2.5f)); };
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.5f, true, 0.3f)); g.drawText ("ENERGY", a.withHeight (16), Justification::centredLeft);
        g.setColour (energyCol (e).withMultipliedBrightness (kk::theme().night ? 1.0f : 0.6f)); g.setFont (kk::modern::font (30.0f, true, 0.12f)); g.drawText (kk::worlds::energyWord (e), a.withTrimmedTop (16).withHeight (36), Justification::centredLeft);
        const int segs = 24; const float sw = a.getWidth() / (float) segs;
        for (int k = 0; k < segs; ++k)
        {
            const float f = (float) k / (float) (segs - 1); const bool on = f <= e + 0.001f;
            g.setColour (on ? energyCol (f) : energyCol (f).withAlpha (0.12f)); g.fillRoundedRectangle (a.getX() + (float) k * sw, a.getY() + 58, sw - 3, 14, 2);
        }
        float y = a.getY() + 90;
        auto row = [&] (const char* n, const char* how, float v, Colour c)
        {
            g.setColour (kk::theme().text); g.setFont (kk::modern::font (12.5f, true, 0.2f)); g.drawText (n, Rectangle<float> (a.getX(), y, 120, 16), Justification::centredLeft);
            g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.0f, true, 0.03f)); g.drawText (how, Rectangle<float> (a.getX() + 110, y, a.getWidth() - 110, 16), Justification::centredRight);
            g.setColour (c.withAlpha (0.15f)); g.fillRoundedRectangle (a.getX(), y + 20, a.getWidth(), 8, 4);
            g.setColour (c); g.fillRoundedRectangle (a.getX(), y + 20, jmax (8.0f, a.getWidth() * v), 8, 4);
            y += 42;
        };
        row ("CROWD", "drag it up", party.crowd, Colour (0xffff4fd8));
        row ("LIGHTS", "drag them up", party.lights, Colour (0xffffc83d));
        row ("DISCO BALL", "spin it in circles", party.spin, Colour (0xff22d3ee));
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.5f, true, 0.03f));
        if (y + 34 < a.getBottom() - 44)
            g.drawFittedText (e < 0.25f ? "deep, warm and sparse - long chords, bass on 1 and 3" : e < 0.5f ? "warm - four on the floor, one chord a bar" : e < 0.75f ? "bright - offbeat stabs, a house bass" : "euphoric - high stabs, pushes, a busy bass",
                              Rectangle<float> (a.getX(), y + 2, a.getWidth(), 34).toNearestInt(), Justification::topLeft, 2, 0.85f);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        tickBase();
        const float e = party.energy();
        beat += (1.0f / 30.0f) * (118.0f + 10.0f * e) / 60.0f;
        if (mode != 3) ballAngle += 0.01f + 0.3f * party.spin;
        repaint (stage()); repaint (panel()); repaint (strip());
    }

    HotButton grooveBtn { lnf };
    kk::worlds::Party party;
    bool groove = false, grooveMine = false;
    int mode = 0;
    float start = 0, lastAng = 0, angVel = 0, ballAngle = 0, beat = 0;
    uint32 lastMs = 0, lastLive = 0;
};
