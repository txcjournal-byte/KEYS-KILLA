// v0.45 COOK page (included by PluginEditor.cpp): "cooking beats" - replaces ALCHEMY + SCULPT. No knobs: a kitchen.
// INGREDIENTS go into the pot (click jars to take several in your hand, drag them in) = what EXCITES the matter and the BODY.
// The gestures are cooking: STIR (circles in the pot), BOIL (hold still in the pot), FRY (pan onto the fire), BAKE (dish into
// the oven - the longer, the longer / warmer / darker), CHOP (knife strokes on the board), BLEND (hold the blender), FREEZE
// (dish into the freezer / hold its door), SEASON (salt = bright, pepper = grit, chilli = aggression, sugar = shine).
// The sound changes at once (animations are only for joy). SERVE puts the dish on the plate and plays a phrase.
// The RECIPE is a SOUND CODE (EV-....) - copy it, type one in, the COOKBOOK keeps your last dishes.
// Also the sound chooser (parents, the EVOLVE seed, the melody sound): then USE IT sends the sound back.
#include "SoundCode.h"

class CookPage : public Component, private Timer
{
public:
    CookPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (serveBtn, "SERVE", "The dish lands on the plate and plays a little phrase (it goes into your COOKBOOK too)", [this] { serve(); });
        serveBtn.hero = true;
        btn (playBtn, "PLAY", "Hear the dish in a little phrase / stop it (it is on your keys too)", [this] { if (phrase) stopPhrase(); else hear(); });
        btn (mutateBtn, "NEW MUTATION", "The same recipe, another mutation of the matter", [this] { rec.seed = (uint16) (1 + Random::getSystemRandom().nextInt (65535)); cookNow (true); });
        btn (plantBtn, "PLANT IN\nEVOLVE", "This dish becomes the seed of a new EVOLVE tree", [this] { if (cur.valid()) { proc.evoSeedGenome (cur); say ("planted - open EVOLVE"); } });
        btn (saveBtn, "SAVE", "Save this dish into your folders / sound kits", [this]
        {
            if (! cur.valid()) return;
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (cur, rate, 3.0), rate, rate, cur.name);
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<CookPage> (this)] (String m) { if (safe != nullptr) safe->say (m); });
        });
        btn (lifeBtn, "TO LIFE", "The dish becomes the sound of the LIFE melodies", [this] { if (! cur.valid()) return; proc.alcUse (cur, false); if (onToLife) onToLife(); else say ("on your keys - open LIFE"); });
        btn (useBtn, "USE IT", "Use this dish", [this] { if (cur.valid() && onPicked) { proc.alcUse (cur, false); auto fn = onPicked; onPicked = nullptr; useBtn.setVisible (false); resized(); fn(); } });
        useBtn.hero = true; useBtn.setVisible (false);
        btn (copyBtn, "COPY CODE", "Copy the recipe code - send it to a friend, type it in later: the exact same sound", [this]
        {
            if (useBase) { say ("add an ingredient first - the keys' own sound has no code"); return; }
            SystemClipboard::copyTextToClipboard (kk::code::encode (rec)); say ("code copied: " + kk::code::encode (rec));
        });
        btn (enterBtn, "ENTER CODE", "Type a recipe code (EV-....) and cook that exact dish", [this] { askCode(); });
        btn (cleanBtn, "CLEAN KITCHEN", "Empty the pot, cool the stove: start a new dish", [this] { clean(); cookNow (false); });
        dragWav.makeFile = [this] { return cur.valid() ? proc.exportGenomeWav (cur) : File(); };
        dragWav.setTooltip ("Drag the dish into FL as a WAV");
        addAndMakeVisible (dragWav);
        loadBook();
        kk::code::Recipe r;
        if (! book.empty() && kk::code::decode (book.front(), r)) load (r);
        else { rec.ing[3] = 1; rec.ing[0] = 1; rec.setAmount (kk::code::stir, 0.25f); load (rec); }
        cur = recipeSound();
        particles.reserve (200);
        startTimerHz (30);
    }
    ~CookPage() override { stopTimer(); }

    std::function<void()> onToLife;   // the lead wires TO LIFE (the dish is already on the keys when it is called)
    // the chooser mode: title + what happens with the sound
    void pick (const String& title, std::function<void()> then) { pickTitle = title; onPicked = std::move (then); useBtn.setVisible (onPicked != nullptr); resized(); repaint(); }
    void visibilityChanged() override
    {
        if (isVisible()) return;
        if (onPicked) { onPicked = nullptr; useBtn.setVisible (false); resized(); }
        if (phrase) stopPhrase();
    }
    // COOK what is on the keys now (from the top bar): the current sound goes into the pot, the cooking shapes it
    void sculptCurrent()
    {
        base = proc.currentGenome(); useBase = base.valid();
        for (auto& a : amt) a = 0;
        rec.ing = {}; panOnFire = false; dishWhere = 0;
        sync(); cur = recipeSound(); repaint();
    }
    // a full kitchen for the snapshots
    void debugCook()
    {
        rec = {}; rec.ing[0] = 2; rec.ing[3] = 1; rec.ing[2] = 1; rec.seed = 4242;
        useBase = false;
        amt = {}; amt[kk::code::stir] = 0.4f; amt[kk::code::fry] = 0.5f; amt[kk::code::bake] = 0.3f; amt[kk::code::boil] = 0.25f; amt[kk::code::chop] = 0.2f;
        amt[kk::code::chilli] = 0.4f; amt[kk::code::sugar] = 0.3f; amt[kk::code::salt] = 0.15f;
        panOnFire = true; dishWhere = 1; hand[8] = true;
        sync(); cur = recipeSound(); plateDrop = 0;
        Random r (11);
        if (book.size() < 6) for (int i = 0; i < 9; ++i) book.push_back (kk::code::encode (kk::code::random (r)));
        book.insert (book.begin(), kk::code::encode (rec));
        swirl = 2.2f; knifeY = 0.3f;
        repaint();
    }

    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("COOK", 24, 12, 120, 40, Justification::centredLeft);
        g.setColour (onPicked ? kk::accentText() : t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawFittedText (onPicked ? pickTitle + "  -  cook it, then USE IT" : String ("no presets.  ingredients into the pot, then cook it:  stir, boil, fry, bake, chop, blend, freeze, season.  it is on your keys at once"),
                          130, 18, getWidth() - 130 - 200, 28, Justification::centredLeft, 1, 0.8f);
        drawShelf (g);
        drawKitchen (g);
        drawServe (g);
        drawBook (g);
        drawCarried (g);
    }
    void resized() override
    {
        cleanBtn.setBounds (getWidth() - 184, 14, 160, 34);
        const auto s = serveArea();
        const int x = s.getX() + 14, w = s.getWidth() - 28, bw = (w - 16) / 3, y1 = codeChip().getBottom() + 10;
        serveBtn.setBounds (x, y1, bw, 40); playBtn.setBounds (x + bw + 8, y1, bw, 40); mutateBtn.setBounds (x + 2 * (bw + 8), y1, bw, 40);
        saveBtn.setBounds (x, y1 + 46, bw, 40); dragWav.setBounds (x + bw + 8, y1 + 44, bw, 44); plantBtn.setBounds (x + 2 * (bw + 8), y1 + 46, bw, 40);
        copyBtn.setBounds (x, y1 + 92, bw, 40); enterBtn.setBounds (x + bw + 8, y1 + 92, bw, 40);
        lifeBtn.setVisible (! useBtn.isVisible());
        lifeBtn.setBounds (x + 2 * (bw + 8), y1 + 92, bw, 40); useBtn.setBounds (lifeBtn.getBounds());
    }

    // ---------------- gestures ----------------
    void mouseDown (const MouseEvent& e) override
    {
        const auto p = e.position;
        mode = mNone; moved = false; downPos = lastPos = p; lastMoveMs = Time::getMillisecondCounter();
        if (bookArea().toFloat().contains (p))
        {
            for (int i = 0; i < (int) book.size(); ++i)
                if (bookChip (i).contains (p))
                {
                    if (e.mods.isPopupMenu()) { bookMenu (i); return; }
                    kk::code::Recipe r; if (kk::code::decode (book[(size_t) i], r)) { load (r); cookNow (true); say ("cooked again from your cookbook"); }
                    return;
                }
            return;
        }
        for (int i = 0; i < kk::code::numIngredients; ++i)
        {
            if (rec.ing[(size_t) i] > 0 && minusBadge (i).contains (p)) { --rec.ing[(size_t) i]; cookNow (true); return; }
            if (jar (i).contains (p)) { mode = mJar; downJar = i; return; }
        }
        for (int i = 0; i < 4; ++i) if (shaker (i).contains (p)) { mode = mShake; shakeIdx = i; season (i, 2); lastDir = 0; return; }
        if (panRect().contains (p)) { mode = mPan; panPos = p; return; }
        if (dishRect().contains (p)) { mode = mDish; dishPos = p; return; }
        if (blenderRect().contains (p)) { mode = mBlend; return; }
        if (freezerRect().contains (p)) { mode = mFreeze; return; }
        if (boardRect().contains (p)) { mode = mChop; knifeX = p.x; strokeFrom = p.y; lastDir = 0; return; }
        if (potRect().expanded (12).contains (p)) { mode = mPot; lastAng = angleInPot (p); return; }
        if (plateRect().contains (p)) { hear(); return; }
        if (rangeFront().contains (p)) { say ("drag the dish (next to the blender) into the oven"); return; }
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const auto p = e.position;
        if (p.getDistanceFrom (downPos) > 6) moved = true;
        const auto now = Time::getMillisecondCounter();
        if (p.getDistanceFrom (lastPos) > 1.5f) lastMoveMs = now;
        switch (mode)
        {
            case mJar: if (moved) { carry = hand; carry[(size_t) downJar] = true; carryPos = p; } break;
            case mPan: panPos = p; break;
            case mDish: dishPos = p; break;
            case mPot:
            {
                const auto c = potRect().getCentre();
                if (p.getDistanceFrom (c) > 10)
                {
                    float a = angleInPot (p), d = a - lastAng;
                    while (d > MathConstants<float>::pi) d -= MathConstants<float>::twoPi;
                    while (d < -MathConstants<float>::pi) d += MathConstants<float>::twoPi;
                    lastAng = a;
                    const float speed = jlimit (0.5f, 2.0f, p.getDistanceFrom (lastPos) / 8.0f);   // fast stirring blends quicker
                    add (kk::code::stir, std::abs (d) * speed / (MathConstants<float>::twoPi * 7.0f));
                    swirl += d;
                }
                break;
            }
            case mChop:
            {
                knifeX = p.x;
                const float dy = p.y - lastPos.y; const int dir = dy > 1.5f ? 1 : dy < -1.5f ? -1 : 0;
                if (dir != 0 && dir != lastDir) { if (lastDir == 1 && std::abs (p.y - strokeFrom) > 24) chopStroke (p); strokeFrom = p.y; lastDir = dir; }
                knifeY = jlimit (0.0f, 1.0f, (p.y - boardRect().getY()) / boardRect().getHeight());
                break;
            }
            case mShake:
            {
                const float dy = p.y - lastPos.y; const int dir = dy > 2 ? 1 : dy < -2 ? -1 : 0;
                if (dir != 0 && lastDir != 0 && dir != lastDir) season (shakeIdx, 1);
                if (dir != 0) lastDir = dir;
                shakeOff = jlimit (-14.0f, 14.0f, p.y - downPos.y);
                break;
            }
            default: break;
        }
        lastPos = p;
        repaint();
    }
    void mouseUp (const MouseEvent& e) override
    {
        const auto p = e.position;
        switch (mode)
        {
            case mJar:
                if (! moved) hand[(size_t) downJar] = ! hand[(size_t) downJar];
                else
                {
                    if (potRect().expanded (40).contains (p))
                    {
                        int n = 0;
                        for (int i = 0; i < kk::code::numIngredients; ++i) if (carry[(size_t) i]) { rec.ing[(size_t) i] = (uint8) jmin (kk::code::maxSpoons, rec.ing[(size_t) i] + 1); ++n; splash (i); }
                        if (useBase) useBase = false;   // ingredients: back to a recipe of your own
                        hand = {};
                        say (n > 1 ? String (n) + " ingredients in the pot - a blend" : "in the pot");
                        cookNow (true);
                    }
                    carry = {};
                }
                break;
            case mPan:
                panOnFire = fireSpot().expanded (30).contains (p);
                if (moved) say (panOnFire ? "frying - the longer, the hotter (double-click the pan = cool it)" : "off the fire");
                hearNow(); break;
            case mDish:
                if (moved) { dishWhere = rangeFront().contains (p) ? 1 : freezerRect().contains (p) ? 2 : 0; say (dishWhere == 1 ? "baking - the longer it stays, the longer, warmer, darker" : dishWhere == 2 ? "freezing - an endless, frozen tail" : "on the counter"); }
                hearNow(); break;
            case mShake: shakeOff = 0; hearNow(); break;
            case mNone: break;
            default: hearNow(); break;
        }
        mode = mNone;
        repaint();
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        const auto p = e.position;
        using namespace kk::code;
        auto reset = [&] (int a, const char* what) { amt[(size_t) a] = 0; sync(); cookNow (true); say (String (what)); };
        for (int i = 0; i < 4; ++i) if (shaker (i).contains (p)) { reset (salt + i, "no seasoning"); return; }
        if (panRect().contains (p)) { panOnFire = false; reset (fry, "the pan cooled down"); return; }
        if (rangeFront().contains (p)) { if (dishWhere == 1) dishWhere = 0; reset (bake, "not baked"); return; }
        if (freezerRect().contains (p)) { if (dishWhere == 2) dishWhere = 0; reset (freeze, "thawed"); return; }
        if (blenderRect().contains (p)) { reset (blend, "not blended"); return; }
        if (boardRect().contains (p)) { reset (chop, "whole again"); return; }
        if (potRect().expanded (12).contains (p)) { amt[stir] = amt[boil] = 0; sync(); cookNow (true); say ("not stirred, not boiled"); }
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (bookArea().contains (e.getPosition())) { bookScroll = jlimit (0, jmax (0, (int) book.size() - 4), bookScroll + (w.deltaY < 0 || w.deltaX < 0 ? 1 : -1)); repaint(); }
    }
    void mouseMove (const MouseEvent& e) override { const int h = jarAt (e.position); if (h != hoverJar) { hoverJar = h; repaint (shelfArea()); } }
    void mouseExit (const MouseEvent&) override { hoverJar = -1; repaint (shelfArea()); }

private:
    enum Mode { mNone, mJar, mPan, mDish, mPot, mChop, mShake, mBlend, mFreeze };
    struct Bit { Point<float> pos, vel; Colour c; float life = 1, size = 3; int kind = 0; };

    // ---------------- layout ----------------
    Rectangle<int> shelfArea() const { return { 24, 92, 236, getHeight() - 92 - 156 }; }
    Rectangle<int> serveArea() const { return { getWidth() - 24 - 384, 92, 384, getHeight() - 92 - 156 }; }
    Rectangle<int> kitchenArea() const { const int x = shelfArea().getRight() + 16; return { x, 92, serveArea().getX() - 16 - x, getHeight() - 92 - 156 }; }
    Rectangle<int> bookArea() const { return { 24, getHeight() - 140, getWidth() - 48, 118 }; }
    Rectangle<float> K (float x, float y, float w, float h) const { const auto k = kitchenArea().toFloat(); return { k.getX() + x * k.getWidth(), k.getY() + y * k.getHeight(), w * k.getWidth(), h * k.getHeight() }; }
    Rectangle<float> jar (int i) const { const auto s = shelfArea().toFloat(); const float w = (s.getWidth() - 28 - 8) / 2.0f, h = (s.getHeight() - 44 - 4 * 8) / 5.0f; return { s.getX() + 14 + (float) (i % 2) * (w + 8), s.getY() + 40 + (float) (i / 2) * (h + 8), w, h }; }
    Rectangle<float> minusBadge (int i) const { const auto j = jar (i); return { j.getRight() - 24, j.getY() + 4, 20, 20 }; }
    int jarAt (Point<float> p) const { for (int i = 0; i < kk::code::numIngredients; ++i) if (jar (i).contains (p)) return i; return -1; }
    Rectangle<float> potRect() const { return K (0.05f, 0.33f, 0.25f, 0.24f); }
    Rectangle<float> fireSpot() const { return K (0.325f, 0.45f, 0.18f, 0.11f); }
    Rectangle<float> panHome() const { return K (0.325f, 0.17f, 0.18f, 0.11f); }
    Rectangle<float> panRect() const { const auto h = panOnFire ? fireSpot() : panHome(); return mode == mPan && moved ? h.withCentre (panPos) : h; }
    Rectangle<float> rangeFront() const { return K (0.02f, 0.60f, 0.50f, 0.39f); }
    Rectangle<float> ovenWindow() const { return K (0.08f, 0.69f, 0.38f, 0.20f); }
    Rectangle<float> rackArea() const { return K (0.545f, 0.03f, 0.29f, 0.17f); }
    Rectangle<float> shaker (int i) const { const auto r = rackArea(); const float w = r.getWidth() / 4.0f; return { r.getX() + (float) i * w + 6, r.getY() + 4, w - 12, r.getHeight() - 22 }; }
    Rectangle<float> blenderRect() const { return K (0.555f, 0.25f, 0.11f, 0.33f); }
    Rectangle<float> dishHome() const { return K (0.69f, 0.46f, 0.13f, 0.10f); }
    Rectangle<float> dishRect() const
    {
        if (mode == mDish && moved) return dishHome().withCentre (dishPos);
        if (dishWhere == 1) return dishHome().withCentre (ovenWindow().getCentre().translated (0, 6));
        if (dishWhere == 2) return dishHome().withWidth (freezerRect().getWidth() * 0.7f).withCentre ({ freezerRect().getCentreX(), freezerRect().getY() + freezerRect().getHeight() * 0.36f });
        return dishHome();
    }
    Rectangle<float> boardRect() const { return K (0.55f, 0.65f, 0.27f, 0.30f); }
    Rectangle<float> freezerRect() const { return K (0.85f, 0.03f, 0.13f, 0.94f); }
    Rectangle<float> plateRect() const { const auto s = serveArea().toFloat(); return { s.getX() + 30, s.getY() + 34, s.getWidth() - 60, s.getHeight() * 0.34f }; }
    Rectangle<int> codeChip() const { const auto s = serveArea(); return { s.getX() + 14, (int) plateRect().getBottom() + 74, s.getWidth() - 28, 32 }; }
    Rectangle<float> bookChip (int i) const { const auto a = bookArea().toFloat(); const int k = i - bookScroll; return { a.getX() + 150 + (float) k * 146.0f, a.getY() + 40, 138, 66 }; }
    float angleInPot (Point<float> p) const { const auto c = potRect().getCentre(); return std::atan2 (p.y - c.y, p.x - c.x); }

    // ---------------- the recipe -> the sound ----------------
    void add (int a, float d) { amt[(size_t) a] = jlimit (0.0f, 1.0f, amt[(size_t) a] + d); sync(); live(); }
    void sync() { for (int a = 0; a < kk::code::numActs; ++a) rec.setAmount (a, amt[(size_t) a]); }
    void load (const kk::code::Recipe& r)
    {
        rec = r; useBase = false;
        for (int a = 0; a < kk::code::numActs; ++a) amt[(size_t) a] = r.amount (a);
        panOnFire = false; dishWhere = 0; hand = {};
    }
    void clean() { rec.ing = {}; amt = {}; sync(); panOnFire = false; dishWhere = 0; hand = {}; useBase = false; say ("a clean kitchen - add ingredients"); }
    KeysKillaProcessor::Genome recipeSound()
    {
        if (useBase)
        {
            const auto c = kk::code::cook (rec);
            const float bo = rec.amount (kk::code::boil);
            auto g = proc.sculpt (base, c.stretch, c.bright - 0.2f * bo, c.heat, c.cool, jmin (1.0f, c.split + 0.3f * bo));
            return g;
        }
        const auto c = kk::code::cook (rec);
        proc.alcExc = c.exc; proc.alcBody = c.body; proc.alcMatter = c.matter; proc.alcSize = c.size; proc.alcSeed = c.seed;
        return kk::code::dish (proc, rec);
    }
    void cookNow (bool audition)
    {
        sync();
        cur = recipeSound(); lastRec = rec;
        proc.alcUse (cur, audition && ! phrase);
        repaint();
    }
    void live()
    {
        const auto now = Time::getMillisecondCounter();
        if (rec != lastRec && now - lastLive > 110) { lastLive = now; lastRec = rec; cur = recipeSound(); proc.alcUse (cur, false); }
    }
    void hearNow() { sync(); if (rec != lastRec || ! cur.valid()) cookNow (true); else proc.alcUse (cur, ! phrase); }
    void season (int i, int steps)
    {
        add (kk::code::salt + i, (float) steps / (float) kk::code::levels);
        const auto s = shaker (i); const auto pot = potRect();
        static const uint32 cols[] { 0xfff4f4f4, 0xff3a3a3a, 0xffff3b2f, 0xffffc4e1 };
        Random r ((int64) Time::getMillisecondCounter());
        for (int k = 0; k < 10 * steps; ++k)
        {
            Bit b; b.pos = { s.getCentreX() + (r.nextFloat() - 0.5f) * 10, s.getBottom() };
            const auto to = Point<float> (pot.getCentreX() + (r.nextFloat() - 0.5f) * pot.getWidth() * 0.6f, pot.getY() + 6);
            b.vel = (to - b.pos) / (18.0f + r.nextFloat() * 8.0f); b.c = Colour (cols[i]); b.size = 2.5f + r.nextFloat() * 2.0f; b.kind = 4; b.life = 1.0f;
            pushBit (b);
        }
    }
    void chopStroke (Point<float> p)
    {
        add (kk::code::chop, 1.0f / (float) kk::code::levels);
        chopFlash = 1.0f;
        Random r ((int64) Time::getMillisecondCounter());
        const Colour c = ingredientTint();
        for (int k = 0; k < 6; ++k) { Bit b; b.pos = { p.x, boardRect().getCentreY() }; b.vel = { (r.nextFloat() - 0.5f) * 6, -2 - r.nextFloat() * 4 }; b.c = c; b.size = 4; b.kind = 5; pushBit (b); }
    }
    void splash (int i)
    {
        Random r (i * 7 + (int) Time::getMillisecondCounter());
        const auto pot = potRect();
        for (int k = 0; k < 14; ++k) { Bit b; b.pos = { pot.getCentreX() + (r.nextFloat() - 0.5f) * 30, pot.getY() + 8 }; b.vel = { (r.nextFloat() - 0.5f) * 7, -3 - r.nextFloat() * 5 }; b.c = Colour (kk::code::ingredients()[(size_t) i].colour); b.size = 4; b.kind = 5; pushBit (b); }
    }
    void pushBit (const Bit& b) { if (particles.size() < 200) particles.push_back (b); }
    void say (const String& s) { note = s; noteMs = Time::getMillisecondCounter(); repaint(); }

    // ---------------- serve / the phrase / the cookbook ----------------
    void serve()
    {
        if (! cur.valid()) return;
        cookNow (false);
        plateDrop = 1.0f;
        if (! useBase) { remember (kk::code::encode (rec)); say ("served - in your cookbook"); }
        hear();
    }
    void hear()
    {
        if (! cur.valid()) return;
        proc.alcUse (cur, false);
        Random r ((int64) rec.seed + 17);
        const auto c = kk::code::cook (rec);
        const bool bass = ! useBase && c.exc == 4;
        static const int shapes[4][8] { { 0, 2, 4, 2, 7, 4, 2, 0 }, { 0, 4, 7, 9, 7, 4, 2, 4 }, { 7, 4, 2, 0, 2, 4, 0, -3 }, { 0, 0, 4, 2, 0, 7, 5, 4 } };
        const auto& sh = shapes[r.nextInt (4)];
        // chopped = short, quick notes; blended / frozen = long ones
        const float lenMul = jlimit (0.35f, 1.6f, 1.0f - 0.6f * rec.amount (kk::code::chop) + 0.6f * (rec.amount (kk::code::blend) + rec.amount (kk::code::freeze)));
        std::vector<kk::LoopNote> ln; float pos = 0;
        for (int i = 0; i < 8; ++i)
        {
            const float len = (i % 4 == 3) ? 1.0f : 0.5f + 0.5f * (float) r.nextInt (2);
            const int pitch = kk::mel::degreeToPitch (sh[i], 9, kk::mel::scMinor, bass ? 0.0f : 0.5f) - (bass ? 12 : 0);
            ln.push_back ({ pos, jmin (len * 0.9f * lenMul, 3.5f), pitch, bass });
            pos += len;
            if (pos >= 8.0f) break;
        }
        proc.playCustomLoop (ln, 8.0);
        phrase = true; playBtn.setButtonText ("STOP"); playBtn.selected = true; playBtn.repaint();
    }
    void stopPhrase()
    {
        if (proc.loopOwnerId() == 4) proc.stopLoop();
        phrase = false; playBtn.setButtonText ("PLAY"); playBtn.selected = false; playBtn.repaint();
    }
    void remember (const String& code)
    {
        book.erase (std::remove (book.begin(), book.end(), code), book.end());
        book.insert (book.begin(), code);
        if (book.size() > 24) book.resize (24);
        bookScroll = 0;
        storeBook();
    }
    void loadBook()
    {
        if (auto s = openSettings())
        {
            StringArray v; v.addTokens (s->getValue ("cookbook"), " ", "");
            kk::code::Recipe r;
            for (auto& c : v) if (kk::code::decode (c, r) && (int) book.size() < 24) book.push_back (c);
        }
    }
    void storeBook() { if (auto s = openSettings()) { StringArray v; for (auto& c : book) v.add (c); s->setValue ("cookbook", v.joinIntoString (" ")); } }
    void bookMenu (int i)
    {
        PopupMenu m;
        m.addItem (1, "COOK IT AGAIN"); m.addItem (2, "COPY CODE"); m.addItem (3, "TAKE OUT OF THE COOKBOOK");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (bookChip (i).toNearestInt())),
            [safe = SafePointer<CookPage> (this), i] (int r)
            {
                if (safe == nullptr || ! isPositiveAndBelow (i, (int) safe->book.size())) return;
                const auto code = safe->book[(size_t) i];
                kk::code::Recipe rc;
                if (r == 1 && kk::code::decode (code, rc)) { safe->load (rc); safe->cookNow (true); }
                if (r == 2) { SystemClipboard::copyTextToClipboard (code); safe->say ("code copied: " + code); }
                if (r == 3) { safe->book.erase (safe->book.begin() + i); safe->storeBook(); safe->repaint(); }
            });
    }
    void askCode()
    {
        auto* w = new AlertWindow ("ENTER A RECIPE CODE", "Type or paste a dish code (EV-....) - you get that exact sound:", MessageBoxIconType::NoIcon);
        w->addTextEditor ("code", "", "Code");
        w->addButton ("COOK IT", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([w, safe = SafePointer<CookPage> (this)] (int r)
        {
            const auto text = w->getTextEditorContents ("code").trim();
            if (r != 1 || safe == nullptr || text.isEmpty()) return;
            kk::code::Recipe rc;
            if (! kk::code::decode (text, rc)) { safe->say ("that is not a dish code - check it for a typo"); return; }
            safe->load (rc); safe->serve();
        }), true);
    }

    // ---------------- drawing ----------------
    Colour ingredientTint() const
    {
        float r = 0, gg = 0, b = 0, w = 0;
        for (int i = 0; i < kk::code::numIngredients; ++i) if (const float k = rec.ing[(size_t) i]; k > 0) { const Colour c (kk::code::ingredients()[(size_t) i].colour); r += k * c.getFloatRed(); gg += k * c.getFloatGreen(); b += k * c.getFloatBlue(); w += k; }
        if (useBase) return Colour (0xffff8a3d);
        if (w <= 0) return Colour (0xff6fb7d8);
        return Colour::fromFloatRGBA (r / w, gg / w, b / w, 1.0f);
    }
    void drawShelf (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = shelfArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("INGREDIENTS", a.reduced (14, 10).withHeight (18).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.02f));
        g.drawText ("click = in your hand, drag into the pot", a.reduced (14, 10).withTrimmedTop (16).withHeight (14).toNearestInt(), Justification::centredLeft);
        const auto& I = kk::code::ingredients();
        for (int i = 0; i < kk::code::numIngredients; ++i)
        {
            const auto r = jar (i);
            const auto c = Colour (I[(size_t) i].colour);
            const bool inHand = hand[(size_t) i], inPot = rec.ing[(size_t) i] > 0, hov = hoverJar == i;
            g.setGradientFill (ColourGradient (c.withAlpha (inHand ? 0.38f : inPot ? 0.22f : hov ? 0.16f : 0.08f), r.getX(), r.getY(), Colours::black.withAlpha (0.18f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 12);
            g.setColour (inHand ? Colours::white : c.withAlpha (inPot ? 0.9f : 0.35f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, inHand ? 2.4f : 1.0f);
            drawIngredient (g, i, r.withTrimmedBottom (r.getHeight() * 0.36f).reduced (r.getWidth() * 0.26f, 4).translated (0, 3), 1.0f);
            g.setColour (t.text); g.setFont (kk::modern::font (10.5f, true, 0.08f));
            g.drawFittedText (I[(size_t) i].name, r.withTrimmedTop (r.getHeight() * 0.64f).reduced (4, 0).withHeight (16).toNearestInt(), Justification::centred, 1, 0.75f);
            g.setColour (t.dim); g.setFont (kk::modern::font (9.0f, true, 0.0f));
            g.drawFittedText (I[(size_t) i].hint, r.withTrimmedTop (r.getHeight() * 0.64f + 15).reduced (4, 0).withHeight (13).toNearestInt(), Justification::centred, 1, 0.7f);
            if (inPot)
            {
                // spoons in the pot + take one out
                for (int k = 0; k < rec.ing[(size_t) i]; ++k) { g.setColour (c); g.fillEllipse (r.getX() + 8 + (float) k * 9, r.getY() + 8, 6, 6); }
                const auto mb = minusBadge (i);
                g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (mb);
                g.setColour (Colours::white.withAlpha (0.85f)); g.drawEllipse (mb.reduced (0.5f), 1.0f);
                g.fillRect (mb.getCentreX() - 4.5f, mb.getCentreY() - 0.8f, 9.0f, 1.6f);
            }
        }
    }
    // the drawn ingredients (vector icons)
    void drawIngredient (Graphics& g, int i, Rectangle<float> r, float alpha) const
    {
        const auto c = Colour (kk::code::ingredients()[(size_t) i].colour).withMultipliedAlpha (alpha);
        const float s = std::min (r.getWidth(), r.getHeight()), cx = r.getCentreX(), cy = r.getCentreY(), w = s * 0.5f;
        const PathStrokeType st (std::max (1.6f, s * 0.05f), PathStrokeType::curved, PathStrokeType::rounded);
        auto shade = [&] (Path& p, Colour col) { g.setGradientFill (ColourGradient (col.brighter (0.5f), cx - w, cy - w, col.darker (0.45f), cx + w, cy + w, false)); g.fillPath (p); };
        Path p;
        switch (i)
        {
            case 0: // metal spoon
            {
                p.addEllipse (cx - w * 0.85f, cy - w * 0.55f, w * 0.75f, w * 0.95f);
                Path h; h.addRoundedRectangle (cx - w * 0.15f, cy - w * 0.12f, w * 1.15f, w * 0.2f, w * 0.1f);
                h.applyTransform (AffineTransform::rotation (0.45f, cx - w * 0.15f, cy));
                shade (p, c); shade (h, c);
                g.setColour (Colours::white.withAlpha (0.7f * alpha)); g.fillEllipse (cx - w * 0.68f, cy - w * 0.4f, w * 0.2f, w * 0.3f);
                break;
            }
            case 1: // bone broth: a bowl with a bone
            {
                p.addPieSegment (cx - w * 0.9f, cy - w * 0.6f, w * 1.8f, w * 1.6f, MathConstants<float>::halfPi, MathConstants<float>::pi * 1.5f, 0.0f);
                shade (p, Colour (0xffc87a3a).withMultipliedAlpha (alpha));
                Path b; const float by = cy - w * 0.15f;
                b.addRoundedRectangle (cx - w * 0.55f, by - w * 0.09f, w * 1.1f, w * 0.18f, w * 0.08f);
                for (float sx : { -0.6f, 0.6f }) { b.addEllipse (cx + sx * w - w * 0.17f, by - w * 0.26f, w * 0.26f, w * 0.26f); b.addEllipse (cx + sx * w - w * 0.17f, by - w * 0.02f, w * 0.26f, w * 0.26f); }
                b.applyTransform (AffineTransform::rotation (-0.35f, cx, by).translated (0, -w * 0.2f));
                shade (b, c);
                break;
            }
            case 2: // sparkling gas: a bottle with bubbles
            {
                p.addRoundedRectangle (cx - w * 0.35f, cy - w * 0.2f, w * 0.7f, w * 1.1f, w * 0.18f);
                p.addRectangle (cx - w * 0.14f, cy - w * 0.62f, w * 0.28f, w * 0.46f);
                shade (p, c);
                g.setColour (Colours::white.withAlpha (0.85f * alpha));
                for (int k = 0; k < 5; ++k) { const float bx = cx + std::sin ((float) k * 2.3f) * w * 0.55f, by = cy - w * 0.95f + std::fmod ((float) k * 0.31f + phase * 0.25f, 1.0f) * -w * 0.1f - (float) k * w * 0.06f; g.drawEllipse (bx - 2, by - 2, 4 + (float) (k % 2) * 2, 4 + (float) (k % 2) * 2, 1.0f); }
                break;
            }
            case 3: // honey: a pot with a drip
            {
                p.addRoundedRectangle (cx - w * 0.7f, cy - w * 0.35f, w * 1.4f, w * 1.15f, w * 0.4f);
                shade (p, c);
                Path lid; lid.addRoundedRectangle (cx - w * 0.78f, cy - w * 0.55f, w * 1.56f, w * 0.26f, w * 0.12f); shade (lid, c.darker (0.25f));
                Path d; d.startNewSubPath (cx - w * 0.2f, cy - w * 0.32f); d.quadraticTo (cx - w * 0.2f, cy + w * 0.1f, cx - w * 0.05f, cy + w * 0.15f); d.quadraticTo (cx + w * 0.1f, cy + w * 0.1f, cx + w * 0.1f, cy - w * 0.32f);
                g.setColour (c.brighter (0.4f)); g.fillPath (d);
                g.setColour (Colours::black.withAlpha (0.3f * alpha)); g.setFont (kk::modern::font (s * 0.2f, true, 0.1f)); g.drawText ("H", Rectangle<float> (cx - w * 0.5f, cy + w * 0.1f, w, w * 0.5f), Justification::centred);
                break;
            }
            case 4: // crystal sugar: an iso cube
            {
                const Point<float> top (cx, cy - w * 0.85f), l (cx - w * 0.75f, cy - w * 0.42f), rr (cx + w * 0.75f, cy - w * 0.42f), m (cx, cy), bl (cx - w * 0.75f, cy + w * 0.45f), br (cx + w * 0.75f, cy + w * 0.45f), bot (cx, cy + w * 0.88f);
                Path f1; f1.addTriangle (top, l, m); f1.addTriangle (top, rr, m); g.setColour (c.brighter (0.6f)); g.fillPath (f1);
                Path f2; f2.startNewSubPath (l); f2.lineTo (m); f2.lineTo (bot); f2.lineTo (bl); f2.closeSubPath(); g.setColour (c); g.fillPath (f2);
                Path f3; f3.startNewSubPath (rr); f3.lineTo (m); f3.lineTo (bot); f3.lineTo (br); f3.closeSubPath(); g.setColour (c.darker (0.4f)); g.fillPath (f3);
                g.setColour (Colours::white.withAlpha (0.8f * alpha)); drawStar (g, { cx + w * 0.55f, cy - w * 0.75f }, w * 0.22f, Colours::white.withAlpha (alpha));
                break;
            }
            case 5: // hot coals with a flame
            {
                for (int k = 0; k < 3; ++k) { Path cp; cp.addEllipse (cx - w * 0.85f + (float) k * w * 0.55f, cy + w * 0.1f - (float) (k % 2) * w * 0.15f, w * 0.65f, w * 0.5f); shade (cp, Colour (0xff3a2a2a).withMultipliedAlpha (alpha)); g.setColour (c.withAlpha (0.6f * alpha)); g.strokePath (cp, PathStrokeType (1.2f)); }
                Path f; const float fl = 0.08f * std::sin (phase * 6.0f);
                f.startNewSubPath (cx, cy - w * (0.95f + fl)); f.cubicTo (cx + w * 0.5f, cy - w * 0.4f, cx + w * 0.35f, cy + w * 0.1f, cx, cy + w * 0.1f); f.cubicTo (cx - w * 0.35f, cy + w * 0.1f, cx - w * 0.5f, cy - w * 0.4f, cx, cy - w * (0.95f + fl));
                g.setGradientFill (ColourGradient (Colour (0xffffe066).withMultipliedAlpha (alpha), cx, cy, c, cx, cy - w, false)); g.fillPath (f);
                break;
            }
            case 6: // breath mint: a leaf + a breath curl
            {
                p.startNewSubPath (cx - w * 0.8f, cy + w * 0.6f); p.cubicTo (cx - w * 0.7f, cy - w * 0.5f, cx + w * 0.3f, cy - w * 0.9f, cx + w * 0.8f, cy - w * 0.8f); p.cubicTo (cx + w * 0.7f, cy, cx, cy + w * 0.7f, cx - w * 0.8f, cy + w * 0.6f); p.closeSubPath();
                shade (p, c);
                Path v; v.startNewSubPath (cx - w * 0.7f, cy + w * 0.55f); v.quadraticTo (cx, cy - w * 0.1f, cx + w * 0.7f, cy - w * 0.72f); g.setColour (Colours::white.withAlpha (0.6f * alpha)); g.strokePath (v, PathStrokeType (1.4f));
                break;
            }
            case 7: // bass flour: a sack with a sub wave
            {
                p.startNewSubPath (cx - w * 0.6f, cy - w * 0.6f); p.quadraticTo (cx, cy - w * 0.8f, cx + w * 0.6f, cy - w * 0.6f); p.quadraticTo (cx + w * 0.95f, cy + w * 0.3f, cx + w * 0.7f, cy + w * 0.85f);
                p.lineTo (cx - w * 0.7f, cy + w * 0.85f); p.quadraticTo (cx - w * 0.95f, cy + w * 0.3f, cx - w * 0.6f, cy - w * 0.6f); p.closeSubPath();
                shade (p, c);
                Path sw; for (int k = 0; k <= 20; ++k) { const float x = cx - w * 0.5f + w * (float) k / 20.0f, y = cy + w * 0.2f + std::sin ((float) k / 20.0f * MathConstants<float>::twoPi) * w * 0.25f; if (k == 0) sw.startNewSubPath (x, y); else sw.lineTo (x, y); }
                g.setColour (Colour (0xff6b4a2a).withMultipliedAlpha (alpha)); g.strokePath (sw, PathStrokeType (std::max (1.6f, s * 0.05f)));
                break;
            }
            case 8: // moon dust: a crescent + stars
            {
                Path moon; moon.addEllipse (cx - w * 0.8f, cy - w * 0.7f, w * 1.4f, w * 1.4f);
                Path cut; cut.addEllipse (cx - w * 0.45f, cy - w * 0.95f, w * 1.4f, w * 1.4f);
                moon.setUsingNonZeroWinding (false); moon.addPath (cut);
                Path disc; disc.addEllipse (cx - w * 0.8f, cy - w * 0.7f, w * 1.4f, w * 1.4f); g.saveState(); g.reduceClipRegion (disc); shade (moon, c); g.restoreState();
                drawStar (g, { cx + w * 0.55f, cy + w * 0.35f }, w * 0.22f, c.brighter (0.5f)); drawStar (g, { cx + w * 0.75f, cy - w * 0.45f }, w * 0.14f, Colours::white.withAlpha (alpha));
                break;
            }
            default: // squid ink: a drop with curls
            {
                p.startNewSubPath (cx, cy - w * 0.9f); p.cubicTo (cx + w * 0.75f, cy - w * 0.05f, cx + w * 0.55f, cy + w * 0.55f, cx, cy + w * 0.55f); p.cubicTo (cx - w * 0.55f, cy + w * 0.55f, cx - w * 0.75f, cy - w * 0.05f, cx, cy - w * 0.9f); p.closeSubPath();
                shade (p, c);
                Path t; for (float sx : { -0.4f, 0.0f, 0.4f }) { t.startNewSubPath (cx + sx * w, cy + w * 0.5f); t.quadraticTo (cx + sx * w + w * 0.2f, cy + w * 0.75f, cx + sx * w - w * 0.05f, cy + w * 0.92f); }
                g.setColour (c); g.strokePath (t, st);
                g.setColour (Colours::white.withAlpha (0.7f * alpha)); g.fillEllipse (cx - w * 0.25f, cy - w * 0.25f, w * 0.18f, w * 0.18f);
                break;
            }
        }
    }
    void meter (Graphics& g, Rectangle<float> r, const String& name, float v, Colour c) const
    {
        // a lit pill: how much of this gesture is in the dish (no numbers)
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        if (v > 0.001f) { g.setColour (c.withAlpha (0.55f + 0.4f * v)); g.fillRoundedRectangle (r.withWidth (jmax (r.getHeight(), r.getWidth() * v)), r.getHeight() * 0.5f); }
        g.setColour (Colours::white.withAlpha (v > 0.001f ? 0.95f : 0.6f)); g.setFont (kk::modern::font (r.getHeight() * 0.62f, true, 0.18f));
        g.drawText (name, r.toNearestInt(), Justification::centred);
    }
    void drawKitchen (Graphics& g)
    {
        using namespace kk::code;
        const auto k = kitchenArea().toFloat();
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (k, 18); g.reduceClipRegion (clip);
        // the wall: warm, a soft tile grid, a neon strip
        const float counterY = K (0, 0.56f, 0, 0).getY();
        g.setGradientFill (ColourGradient (Colour (0xff2a1631), k.getX(), k.getY(), Colour (0xff4a2228), k.getX(), counterY, false)); g.fillRect (k.withBottom (counterY));
        g.setColour (Colour (0x10ffd0a0));
        for (float x = k.getX() + 30; x < k.getRight(); x += 46) g.drawVerticalLine ((int) x, k.getY(), counterY);
        for (float y = k.getY() + 30; y < counterY; y += 46) g.drawHorizontalLine ((int) y, k.getX(), k.getRight());
        // a round window: a futuristic night city
        {
            const auto w = K (0.355f, 0.035f, 0.13f, 0.0f).withHeight (K (0, 0, 0.13f, 0).getWidth());
            g.setGradientFill (ColourGradient (Colour (0xff1b1050), w.getCentreX(), w.getY(), Colour (0xffff6a3d), w.getCentreX(), w.getBottom(), false)); g.fillEllipse (w);
            g.saveState(); Path wc; wc.addEllipse (w); g.reduceClipRegion (wc);
            Random rr (5);
            for (int i = 0; i < 9; ++i) { const float bw = w.getWidth() * (0.08f + 0.08f * rr.nextFloat()), bh = w.getHeight() * (0.2f + 0.45f * rr.nextFloat()); g.setColour (Colour (0xff120a20)); g.fillRect (w.getX() + (float) i * w.getWidth() / 8.5f, w.getBottom() - bh, bw, bh);
                                     g.setColour (Colour (0xffffd27a).withAlpha (0.8f)); g.fillRect (w.getX() + (float) i * w.getWidth() / 8.5f + 2, w.getBottom() - bh + 4, 2.0f, 2.0f); }
            g.setColour (Colours::white.withAlpha (0.8f)); g.fillEllipse (w.getX() + w.getWidth() * 0.65f, w.getY() + w.getHeight() * 0.18f, 6, 6);
            g.restoreState();
            g.setColour (Colour (0xffb9a5c8)); g.drawEllipse (w, 4.0f);
        }
        // neon sign
        g.setColour (Colour (0xffff7ab8).withAlpha (0.16f)); g.fillRoundedRectangle (K (0.05f, 0.05f, 0.24f, 0.09f), 10);
        g.setColour (Colour (0xffff7ab8)); g.setFont (kk::modern::font (jmin (22.0f, K (0, 0, 0, 0.045f).getHeight()), true, 0.2f));
        g.drawFittedText ("SOUND KITCHEN", K (0.05f, 0.05f, 0.24f, 0.09f).reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.5f);
        // the counter + cabinets
        g.setGradientFill (ColourGradient (Colour (0xff201521), k.getX(), counterY, Colour (0xff120c14), k.getX(), k.getBottom(), false)); g.fillRect (k.withTop (counterY));
        for (float x = K (0.535f, 0, 0, 0).getX(); x < freezerRect().getX() - 10; x += K (0, 0, 0.155f, 0).getWidth()) { g.setColour (Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (x + 4, counterY + 22, K (0, 0, 0.145f, 0).getWidth(), k.getBottom() - counterY - 30, 6, 1.0f); }
        g.setGradientFill (ColourGradient (Colour (0xff9aa0ad), k.getX(), counterY, Colour (0xff4d505a), k.getX(), counterY + 14, false)); g.fillRect (k.getX(), counterY, k.getWidth(), 14.0f);
        g.setColour (Colour (0xffffa94d).withAlpha (0.8f)); g.fillRect (k.getX(), counterY + 13, k.getWidth(), 2.0f);
        g.setColour (Colour (0xffffa94d).withAlpha (0.12f)); g.fillRect (k.getX(), counterY + 15, k.getWidth(), 10.0f);

        drawRange (g);
        drawPot (g);
        drawPan (g);
        drawRack (g);
        drawBlender (g);
        drawBoard (g);
        drawFreezer (g);
        drawDish (g, dishRect());
        // the particles
        for (auto& b : particles)
        {
            g.setColour (b.c.withAlpha (jlimit (0.0f, 1.0f, b.life)));
            if (b.kind == 4) g.fillEllipse (b.pos.x - b.size * 0.5f, b.pos.y - b.size * 0.5f, b.size, b.size);
            else g.fillRoundedRectangle (b.pos.x - b.size * 0.5f, b.pos.y - b.size * 0.5f, b.size, b.size * 0.8f, 1.0f);
        }
    }
    void drawRange (Graphics& g)
    {
        using namespace kk::code;
        const auto f = rangeFront();
        g.setGradientFill (ColourGradient (Colour (0xff3a3442), f.getX(), f.getY(), Colour (0xff1c1922), f.getX(), f.getBottom(), false)); g.fillRoundedRectangle (f, 10);
        g.setColour (Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (f.reduced (0.5f), 10, 1.0f);
        // the oven: its window glows with the baking
        const float bk = amt[bake], inOven = dishWhere == 1 ? 1.0f : 0.0f;
        const auto w = ovenWindow();
        g.setColour (Colour (0xff0c0910)); g.fillRoundedRectangle (w, 10);
        g.setGradientFill (ColourGradient (Colour (0xffff7a2f).withAlpha (0.15f + 0.6f * jmax (bk, inOven * 0.6f) * (0.85f + 0.15f * std::sin (phase * 3.0f))), w.getCentreX(), w.getBottom(), Colour (0x00ff7a2f), w.getCentreX(), w.getY(), false));
        g.fillRoundedRectangle (w, 10);
        for (int i = 0; i < 3; ++i) { const float y = w.getBottom() - 8 - (float) i * 4; g.setColour (Colour (0xffff5a1f).withAlpha (0.3f + 0.6f * inOven)); g.drawHorizontalLine ((int) y, w.getX() + 14, w.getRight() - 14); }
        g.setColour (Colours::white.withAlpha (0.08f)); g.drawLine (w.getX() + 20, w.getY() + 8, w.getX() + 60, w.getBottom() - 8, 6.0f);
        g.setColour (Colour (0xffb9bcc6)); g.fillRoundedRectangle (w.getX() + 30, w.getY() - 12, w.getWidth() - 60, 6, 3);
        meter (g, { f.getX() + 14, f.getBottom() - 22, f.getWidth() - 28, 15 }, inOven > 0 ? "BAKING" : "BAKE  -  drag the dish in", bk, Colour (0xffff8a3d));
        // the stove top: two rings
        auto ring = [&] (Rectangle<float> r, float heat)
        {
            g.setColour (Colour (0xff0d0b10)); g.fillEllipse (r);
            if (heat > 0.01f) { g.setColour (Colour (0xffff3b1f).withAlpha (0.25f + 0.6f * heat)); g.drawEllipse (r.reduced (r.getWidth() * 0.12f, r.getHeight() * 0.18f), 3.0f); g.drawEllipse (r.reduced (r.getWidth() * 0.28f, r.getHeight() * 0.36f), 2.0f); }
            else { g.setColour (Colours::white.withAlpha (0.15f)); g.drawEllipse (r.reduced (r.getWidth() * 0.12f, r.getHeight() * 0.18f), 1.5f); }
        };
        const auto pr = potRect(), fs = fireSpot();
        ring ({ pr.getX() + 8, f.getY() - 12, pr.getWidth() - 16, 20 }, jmax (amt[boil], 0.15f));
        ring ({ fs.getX() + 4, f.getY() - 12, fs.getWidth() - 8, 20 }, panOnFire ? 0.6f + 0.4f * amt[fry] : 0.0f);
        // flames under the pan
        if (panOnFire)
        {
            for (int i = 0; i < 9; ++i)
            {
                const float x = fs.getX() + 14 + (float) i * (fs.getWidth() - 28) / 8.0f, hgt = (10 + 14 * amt[fry]) * (0.7f + 0.3f * std::sin (phase * 9.0f + (float) i * 1.7f));
                Path fl; fl.startNewSubPath (x - 5, f.getY() - 4); fl.quadraticTo (x - 4, f.getY() - 4 - hgt * 0.6f, x, f.getY() - 4 - hgt); fl.quadraticTo (x + 4, f.getY() - 4 - hgt * 0.6f, x + 5, f.getY() - 4); fl.closeSubPath();
                g.setGradientFill (ColourGradient (Colour (0xffffe066), x, f.getY() - 4, Colour (0xffff3b1f).withAlpha (0.2f), x, f.getY() - 4 - hgt, false)); g.fillPath (fl);
            }
        }
    }
    void drawPot (Graphics& g)
    {
        using namespace kk::code;
        const auto r = potRect();
        const auto tint = ingredientTint();
        const float bo = amt[boil], heatLook = jmax (bo, amt[fry] * 0.4f);
        // steam
        for (int i = 0; i < 5; ++i)
        {
            const float u = std::fmod (phase * (0.25f + 0.35f * bo) + (float) i * 0.2f, 1.0f);
            const float x = r.getX() + r.getWidth() * (0.25f + 0.12f * (float) i) + std::sin (u * 6.0f + (float) i) * 12.0f, y = r.getY() - u * r.getHeight() * (0.8f + 0.6f * heatLook);
            g.setColour (Colours::white.withAlpha ((0.06f + 0.16f * heatLook) * (1.0f - u))); g.fillEllipse (x - 10 - u * 14, y - 10 - u * 14, 20 + u * 28, 20 + u * 28);
        }
        // the body
        Path body; body.addRoundedRectangle (r.getX(), r.getY() + 10, r.getWidth(), r.getHeight() - 10, 18, 18, false, false, true, true);
        g.setGradientFill (ColourGradient (Colour (0xffc9ced8), r.getX(), r.getY(), Colour (0xff5a5f6c), r.getRight(), r.getBottom(), false)); g.fillPath (body);
        g.setColour (Colours::white.withAlpha (0.35f)); g.fillRoundedRectangle (r.getX() + 14, r.getY() + 24, 8, r.getHeight() - 44, 4);
        g.setColour (Colour (0xff6f7480)); g.fillRoundedRectangle (r.getX() - 16, r.getY() + 26, 20, 10, 5); g.fillRoundedRectangle (r.getRight() - 4, r.getY() + 26, 20, 10, 5);
        // the brew
        const auto top = Rectangle<float> (r.getX(), r.getY(), r.getWidth(), 22);
        g.setColour (Colour (0xff2a2d35)); g.fillEllipse (top.expanded (2, 2));
        g.setGradientFill (ColourGradient (tint.brighter (0.3f), top.getCentreX(), top.getY(), tint.darker (0.5f), top.getCentreX(), top.getBottom(), false));
        g.fillEllipse (top.reduced (6, 3));
        // stirring swirl
        g.setColour (Colours::white.withAlpha (0.25f + 0.4f * amt[stir]));
        for (int k = 0; k < 2 + (int) (amt[stir] * 4); ++k)
        {
            Path s; const float a0 = swirl + (float) k * 1.6f;
            s.addCentredArc (top.getCentreX(), top.getCentreY(), top.getWidth() * (0.12f + 0.07f * (float) k), top.getHeight() * (0.12f + 0.06f * (float) k), 0, a0, a0 + 1.6f, true);
            g.strokePath (s, PathStrokeType (1.5f));
        }
        // floating chunks: one per spoon
        int n = 0;
        for (int i = 0; i < numIngredients; ++i)
            for (int s = 0; s < rec.ing[(size_t) i]; ++s, ++n)
            {
                const float a = swirl * 0.5f + (float) n * 2.4f;
                const auto c = Point<float> (top.getCentreX() + std::cos (a) * top.getWidth() * 0.3f, top.getCentreY() + std::sin (a) * top.getHeight() * 0.22f - 6);
                drawIngredient (g, i, Rectangle<float> (22, 22).withCentre (c), 0.95f);
            }
        if (useBase) { g.setColour (Colours::white); g.setFont (kk::modern::font (11.0f, true, 0.15f)); g.drawText ("YOUR KEYS' SOUND", top.translated (0, -26).toNearestInt(), Justification::centred); }
        // bubbles of the boil
        for (int i = 0; i < (int) (bo * 14); ++i)
        {
            const float u = std::fmod (phase * 0.9f + (float) i * 0.37f, 1.0f);
            const float x = top.getX() + top.getWidth() * (0.15f + 0.7f * std::fmod ((float) i * 0.618f, 1.0f)), y = top.getCentreY() - u * 18;
            g.setColour (tint.brighter (0.6f).withAlpha (0.8f * (1.0f - u))); g.drawEllipse (x - 3 - u * 3, y - 3 - u * 3, 6 + u * 6, 6 + u * 6, 1.4f);
        }
        // the ladle while you stir
        if (mode == mPot && moved)
        {
            const auto c = top.getCentre(); const float a = lastAng;
            const auto tip = Point<float> (c.x + std::cos (a) * top.getWidth() * 0.28f, c.y + std::sin (a) * top.getHeight() * 0.3f);
            g.setColour (Colour (0xffe8d2b0)); g.drawLine (tip.x, tip.y, tip.x + 20, tip.y - 70, 6.0f); g.fillEllipse (tip.x - 8, tip.y - 5, 16, 10);
        }
        meter (g, { r.getX() + 6, r.getBottom() - 46, r.getWidth() * 0.5f - 9, 15 }, "STIR", amt[stir], Colour (0xff8f7cff));
        meter (g, { r.getCentreX() + 3, r.getBottom() - 46, r.getWidth() * 0.5f - 9, 15 }, "BOIL", amt[boil], Colour (0xff3dd6c6));
        g.setColour (Colours::black.withAlpha (0.6f)); g.setFont (kk::modern::font (10.5f, true, 0.1f));
        g.drawText ("circles = stir   hold = boil", Rectangle<float> (r.getX(), r.getBottom() - 26, r.getWidth(), 14).toNearestInt(), Justification::centred);
    }
    void drawPan (Graphics& g)
    {
        using namespace kk::code;
        const auto r = panRect();
        const bool home = ! panOnFire && ! (mode == mPan && moved);
        if (home) { g.setColour (Colour (0xffb9a5c8)); g.fillEllipse (r.getRight() - 10, r.getY() - 12, 8, 8); g.drawLine (r.getRight() - 6, r.getY() - 6, r.getRight() - 14, r.getCentreY() - 2, 2.0f); }
        const auto pan = r.withTrimmedRight (r.getWidth() * 0.3f);
        // the handle
        g.setColour (Colour (0xff2a2228)); g.fillRoundedRectangle (pan.getRight() - 6, pan.getCentreY() - 5, r.getWidth() * 0.32f, 10, 5);
        g.setGradientFill (ColourGradient (Colour (0xff4a4a55), pan.getX(), pan.getY(), Colour (0xff15151a), pan.getRight(), pan.getBottom(), false)); g.fillEllipse (pan);
        const float fr = amt[fry];
        const auto in = pan.reduced (pan.getWidth() * 0.1f, pan.getHeight() * 0.14f);
        g.setGradientFill (ColourGradient (Colour (0xff2b2b33).interpolatedWith (Colour (0xffd98a2b), fr), in.getCentreX(), in.getY(), Colour (0xff1a1a20).interpolatedWith (Colour (0xff8a3a10), fr), in.getCentreX(), in.getBottom(), false));
        g.fillEllipse (in);
        if (panOnFire) for (int i = 0; i < 8; ++i)   // sizzle
        {
            const float a = (float) i * 0.8f + phase * 2.0f, d = 0.3f + 0.6f * std::fmod ((float) i * 0.37f + phase * 0.5f, 1.0f);
            g.setColour (Colour (0xffffe9a8).withAlpha (0.7f)); g.fillEllipse (in.getCentreX() + std::cos (a) * in.getWidth() * 0.4f * d - 1.5f, in.getCentreY() + std::sin (a) * in.getHeight() * 0.35f * d - 1.5f, 3, 3);
        }
        const auto fs = fireSpot(); meter (g, { fs.getX() + 4, K (0, 0.335f, 0, 0).getY(), fs.getWidth() - 8, 15 }, panOnFire ? "FRYING" : "FRY  -  pan onto the fire", fr, Colour (0xffff5a1f));
    }
    void drawRack (Graphics& g)
    {
        using namespace kk::code;
        const auto r = rackArea();
        g.setColour (Colour (0xff6b4a3a)); g.fillRoundedRectangle (r.getX(), r.getBottom() - 20, r.getWidth(), 7, 3);
        static const uint32 body[] { 0xfff4f4f4, 0xff3c3c44, 0xffe8342a, 0xffffc4e1 };
        static const char* names[] { "SALT", "PEPPER", "CHILLI", "SUGAR" };
        for (int i = 0; i < 4; ++i)
        {
            auto s = shaker (i).translated (0, mode == mShake && shakeIdx == i ? shakeOff : 0.0f);
            const auto c = Colour (body[i]);
            const auto b = s.withTrimmedTop (s.getHeight() * 0.28f).reduced (s.getWidth() * 0.12f, 0);
            if (i == 2) { Path f; f.addEllipse (b.withTrimmedTop (b.getHeight() * 0.2f)); f.addRoundedRectangle (b.getCentreX() - b.getWidth() * 0.2f, b.getY() - 2, b.getWidth() * 0.4f, b.getHeight() * 0.4f, 3); g.setGradientFill (ColourGradient (c.brighter (0.3f), b.getX(), b.getY(), c.darker (0.5f), b.getRight(), b.getBottom(), false)); g.fillPath (f); }
            else { g.setGradientFill (ColourGradient (c.brighter (0.2f), b.getX(), b.getY(), c.darker (0.4f), b.getRight(), b.getBottom(), false)); g.fillRoundedRectangle (b, 7); }
            const auto cap = Rectangle<float> (b.getX() + 2, s.getY() + s.getHeight() * 0.12f, b.getWidth() - 4, s.getHeight() * 0.18f);
            g.setColour (Colour (0xffc0c4cc)); g.fillRoundedRectangle (cap, 4);
            g.setColour (Colour (0xff3a3a40)); for (int h = 0; h < 3; ++h) g.fillEllipse (cap.getX() + cap.getWidth() * (0.25f + 0.25f * (float) h) - 1.5f, cap.getCentreY() - 1.5f, 3, 3);
            g.setColour (i == 0 || i == 3 ? Colour (0xff302830) : Colours::white); g.setFont (kk::modern::font (9.5f, true, 0.1f));
            g.drawText (String::charToString (names[i][0]), b.toNearestInt(), Justification::centred);
            meter (g, { s.getX() - 4, rackArea().getBottom() - 11, s.getWidth() + 8, 12 }, names[i], amt[(size_t) (salt + i)], c.getBrightness() > 0.8f ? Colour (0xffd8c8b0) : c);
        }
    }
    void drawBlender (Graphics& g)
    {
        using namespace kk::code;
        const auto r = blenderRect();
        const float bl = amt[blend]; const bool on = mode == mBlend;
        const auto baseR = r.withTrimmedTop (r.getHeight() * 0.74f);
        const auto jug = r.withBottom (baseR.getY()).withTrimmedTop (4);
        Path j; j.startNewSubPath (jug.getX(), jug.getY()); j.lineTo (jug.getRight(), jug.getY()); j.lineTo (jug.getRight() - jug.getWidth() * 0.14f, jug.getBottom()); j.lineTo (jug.getX() + jug.getWidth() * 0.14f, jug.getBottom()); j.closeSubPath();
        g.setColour (Colour (0x30c8e8ff)); g.fillPath (j);
        // the drink inside (the pot's colour), a vortex while it runs
        const float lvl = 0.35f + 0.4f * bl;
        g.saveState(); g.reduceClipRegion (j);
        g.setColour (ingredientTint().withAlpha (0.7f)); g.fillRect (jug.withTrimmedTop (jug.getHeight() * (1.0f - lvl)));
        if (on || bl > 0.05f) for (int k = 0; k < 4; ++k) { Path v; const float y = jug.getBottom() - jug.getHeight() * lvl * (0.2f + 0.2f * (float) k); v.addCentredArc (jug.getCentreX(), y, jug.getWidth() * (0.35f - 0.06f * (float) k), 5, 0, phase * (on ? 9.0f : 1.0f) + (float) k, phase * (on ? 9.0f : 1.0f) + (float) k + 3.0f, true); g.setColour (Colours::white.withAlpha (0.4f)); g.strokePath (v, PathStrokeType (1.4f)); }
        g.restoreState();
        g.setColour (Colours::white.withAlpha (0.55f)); g.strokePath (j, PathStrokeType (1.6f));
        g.setColour (Colour (0xff3a3442)); g.fillRoundedRectangle (jug.getX() - 2, jug.getY() - 8, jug.getWidth() + 4, 9, 4);
        g.setGradientFill (ColourGradient (Colour (0xffff7ab8), baseR.getX(), baseR.getY(), Colour (0xff8f3a6a), baseR.getRight(), baseR.getBottom(), false)); g.fillRoundedRectangle (baseR, 8);
        g.setColour (on ? Colour (0xffffe066) : Colours::white.withAlpha (0.5f)); g.fillEllipse (baseR.getCentreX() - 7, baseR.getCentreY() - 7, 14, 14);
        meter (g, { r.getX() - 8, r.getBottom() + 5, r.getWidth() + 16, 15 }, on ? "BLENDING" : "BLEND", bl, Colour (0xffff7ab8));
    }
    void drawBoard (Graphics& g)
    {
        using namespace kk::code;
        const auto r = boardRect();
        g.setGradientFill (ColourGradient (Colour (0xffc98a52), r.getX(), r.getY(), Colour (0xff8a5530), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 14);
        g.setColour (Colour (0xff6e4024).withAlpha (0.5f)); for (int i = 1; i < 6; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 6.0f), r.getX() + 16, r.getRight() - 16);
        g.setColour (Colour (0xff5a3018)); g.fillEllipse (r.getRight() - 22, r.getY() + 10, 10, 10);
        // the chopped pieces: more strokes = finer pieces
        const auto tint = ingredientTint();
        const int pieces = 1 + (int) (amt[chop] * 14);
        Random rr (77);
        for (int i = 0; i < pieces; ++i)
        {
            const float pw = r.getWidth() * 0.5f / (float) (1 + pieces / 3);
            const float x = r.getX() + 20 + rr.nextFloat() * (r.getWidth() * 0.55f), y = r.getY() + 22 + rr.nextFloat() * (r.getHeight() - 60);
            g.setColour (tint.withAlpha (0.9f)); g.fillRoundedRectangle (x, y, jmax (6.0f, pw), jmax (5.0f, pw * 0.6f), 2);
        }
        // the knife
        const float kx = mode == mChop ? jlimit (r.getX() + 10, r.getRight() - 60, knifeX) : r.getX() + r.getWidth() * 0.62f;
        const float ky = r.getY() + r.getHeight() * (mode == mChop ? knifeY * 0.7f : 0.2f);
        Path blade; blade.startNewSubPath (kx, ky); blade.lineTo (kx + 16, ky); blade.lineTo (kx + 16, ky + r.getHeight() * 0.5f); blade.quadraticTo (kx + 4, ky + r.getHeight() * 0.42f, kx, ky + 6); blade.closeSubPath();
        g.setGradientFill (ColourGradient (Colour (0xfff0f3f8), kx, ky, Colour (0xff9aa0ad), kx + 16, ky, false)); g.fillPath (blade);
        g.setColour (Colour (0xff2a2228)); g.fillRoundedRectangle (kx + 1, ky - 34, 14, 36, 5);
        if (chopFlash > 0) { g.setColour (Colours::white.withAlpha (chopFlash * 0.7f)); g.drawLine (kx - 12, ky + r.getHeight() * 0.5f, kx + 30, ky + r.getHeight() * 0.5f, 2.0f); }
        meter (g, { r.getX() + 10, r.getBottom() - 22, r.getWidth() - 20, 15 }, "CHOP  -  knife strokes", amt[chop], Colour (0xffffd23f));
    }
    void drawFreezer (Graphics& g)
    {
        using namespace kk::code;
        const auto r = freezerRect();
        const float fz = amt[freeze]; const bool in = dishWhere == 2, held = mode == mFreeze;
        g.setGradientFill (ColourGradient (Colour (0xffd6f1ff), r.getX(), r.getY(), Colour (0xff6aa7c8), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 12);
        g.setColour (Colour (0xff3d6f8c)); g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * 0.62f), r.getX() + 4, r.getRight() - 4);
        g.fillRoundedRectangle (r.getX() + 8, r.getY() + r.getHeight() * 0.66f, 6, r.getHeight() * 0.18f, 3);
        g.fillRoundedRectangle (r.getX() + 8, r.getY() + r.getHeight() * 0.08f, 6, r.getHeight() * 0.18f, 3);
        // the window, frosting over
        const auto w = Rectangle<float> (r.getX() + 18, r.getY() + r.getHeight() * 0.18f, r.getWidth() - 28, r.getHeight() * 0.36f);
        g.setColour (Colour (0xff0e2a3c)); g.fillRoundedRectangle (w, 8);
        g.setColour (Colour (0xff9be7ff).withAlpha (0.15f + 0.5f * fz)); g.fillRoundedRectangle (w, 8);
        for (int i = 0; i < 14; ++i)
        {
            Random rr ((int64) i * 7919 + 41); rr.nextInt(); rr.nextInt();
            const float u = std::fmod (rr.nextFloat() + phase * (in || held ? 0.12f : 0.03f), 1.0f);
            const auto p = Point<float> (w.getX() + 6 + rr.nextFloat() * (w.getWidth() - 12), w.getY() + 6 + u * (w.getHeight() - 12));
            const float s = 3 + 3 * rr.nextFloat();
            g.setColour (Colours::white.withAlpha ((in || held ? 0.85f : 0.35f) * (0.4f + 0.6f * fz + 0.3f))); g.drawLine (p.x - s, p.y, p.x + s, p.y, 1.0f); g.drawLine (p.x, p.y - s, p.x, p.y + s, 1.0f); g.drawLine (p.x - s * 0.7f, p.y - s * 0.7f, p.x + s * 0.7f, p.y + s * 0.7f, 1.0f);
        }
        g.setColour (Colour (0xff0e2a3c)); g.setFont (kk::modern::font (12.0f, true, 0.2f));
        g.drawText ("FREEZER", Rectangle<float> (r.getX(), r.getY() + r.getHeight() * 0.56f, r.getWidth(), 16).toNearestInt(), Justification::centred);
        g.setColour (Colour (0xff0e2a3c).withAlpha (0.75f)); g.setFont (kk::modern::font (9.5f, true, 0.05f));
        g.drawFittedText ("drop the dish in\nor hold the door", Rectangle<float> (r.getX() + 4, r.getY() + r.getHeight() * 0.7f, r.getWidth() - 8, 30).toNearestInt(), Justification::centred, 2, 0.8f);
        meter (g, { r.getX() + 6, r.getBottom() - 24, r.getWidth() - 12, 15 }, "FREEZE", fz, Colour (0xff5ad1ff));
    }
    void drawDish (Graphics& g, Rectangle<float> r) const
    {
        const auto tint = ingredientTint();
        const float bk = amt[kk::code::bake], fz = amt[kk::code::freeze];
        g.setColour (Colours::black.withAlpha (0.3f)); g.fillEllipse (r.getX() + 4, r.getBottom() - 6, r.getWidth() - 8, 10);
        g.setGradientFill (ColourGradient (Colour (0xffe8693a), r.getX(), r.getY(), Colour (0xff9a3a1a), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r.withTrimmedTop (r.getHeight() * 0.35f), 8);
        const auto food = r.withTrimmedBottom (r.getHeight() * 0.45f).reduced (r.getWidth() * 0.08f, 0).translated (0, r.getHeight() * 0.22f);
        const auto fc = tint.interpolatedWith (Colour (0xff7a3a12), bk * 0.7f).interpolatedWith (Colour (0xffd6f4ff), fz * 0.6f);
        g.setGradientFill (ColourGradient (fc.brighter (0.3f), food.getCentreX(), food.getY(), fc.darker (0.3f), food.getCentreX(), food.getBottom(), false)); g.fillEllipse (food);
        g.setColour (Colour (0xffffd9b8)); g.fillRoundedRectangle (r.getX() - 6, r.getY() + r.getHeight() * 0.4f, 8, 6, 3); g.fillRoundedRectangle (r.getRight() - 2, r.getY() + r.getHeight() * 0.4f, 8, 6, 3);
        if (dishWhere == 0 && ! (mode == mDish && moved)) { g.setColour (Colours::white.withAlpha (0.7f)); g.setFont (kk::modern::font (9.5f, true, 0.15f)); g.drawText ("THE DISH", r.translated (0, r.getHeight() * 0.6f).withHeight (12).toNearestInt(), Justification::centred); }
    }
    void drawServe (Graphics& g)
    {
        using namespace kk::code;
        const auto& t = kk::theme();
        const auto a = serveArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("THE PLATE", a.reduced (14, 10).withHeight (18).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.02f));
        g.drawText ("click the plate = taste it", a.reduced (14, 10).withHeight (18).toNearestInt(), Justification::centredRight);
        // the plate + the dish dome (dropped in by SERVE)
        const auto pr = plateRect();
        const auto plate = pr.withSizeKeepingCentre (pr.getWidth(), pr.getHeight() * 0.62f).translated (0, pr.getHeight() * 0.16f);
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (plate.translated (0, 8));
        g.setGradientFill (ColourGradient (Colour (0xfff7f4ef), plate.getX(), plate.getY(), Colour (0xffb8b2aa), plate.getRight(), plate.getBottom(), false)); g.fillEllipse (plate);
        g.setColour (Colour (0xffffffff)); g.fillEllipse (plate.reduced (plate.getWidth() * 0.12f, plate.getHeight() * 0.14f));
        g.setColour (Colour (0xffff8a3d).withAlpha (0.6f)); g.drawEllipse (plate.reduced (plate.getWidth() * 0.06f, plate.getHeight() * 0.08f), 1.5f);
        const auto ck = cook (rec);
        const float drop = plateDrop * plateDrop;
        const auto dome = Rectangle<float> (plate.getWidth() * 0.42f, plate.getHeight() * 0.78f).withCentre ({ plate.getCentreX(), plate.getCentreY() - plate.getHeight() * 0.3f - drop * 120.0f });
        const auto ca = ck.main >= 0 && ! useBase ? Colour (ingredients()[(size_t) ck.main].colour) : ingredientTint();
        const auto cb = ck.second >= 0 && ! useBase ? Colour (ingredients()[(size_t) ck.second].colour) : ca.darker (0.3f);
        const float bk = amt[bake], fz = amt[freeze], fr = amt[fry];
        Path blob; const int n = 60;   // the food: a wobbly dome (boil = it wobbles)
        for (int i = 0; i <= n; ++i)
        {
            const float u = (float) i / (float) n, ang = MathConstants<float>::pi * (1.0f + u);
            const float wob = 1.0f + (0.03f + 0.08f * amt[boil]) * std::sin (u * 18.0f + phase * 3.0f) - 0.12f * amt[chop] * (float) ((i / 6) % 2);
            const auto pt = Point<float> (dome.getCentreX() + std::cos (ang) * dome.getWidth() * 0.5f * wob, dome.getBottom() + std::sin (ang) * dome.getHeight() * wob);
            if (i == 0) blob.startNewSubPath (pt); else blob.lineTo (pt);
        }
        blob.closeSubPath();
        const auto fa = ca.interpolatedWith (Colour (0xff6a3010), bk * 0.6f).interpolatedWith (Colour (0xffe0f6ff), fz * 0.5f), fb = cb.interpolatedWith (Colour (0xff3a1a08), bk * 0.6f).interpolatedWith (Colour (0xff9be7ff), fz * 0.5f);
        g.setColour (fa.withAlpha (0.25f)); g.fillPath (blob, AffineTransform::scale (1.15f, 1.12f, dome.getCentreX(), dome.getBottom()));
        g.setGradientFill (ColourGradient (fa.brighter (0.4f), dome.getX(), dome.getY(), fb.darker (0.2f), dome.getRight(), dome.getBottom(), false)); g.fillPath (blob);
        if (fr > 0.05f) { g.setColour (Colour (0xffd98a2b).withAlpha (0.6f * fr)); g.strokePath (blob, PathStrokeType (3.0f)); }   // fried: a golden crust
        g.setColour (Colours::white.withAlpha (0.35f)); g.fillEllipse (dome.getX() + dome.getWidth() * 0.22f, dome.getY() + dome.getHeight() * 0.2f, dome.getWidth() * 0.16f, dome.getHeight() * 0.12f);
        // the seasoning on top
        Random rr ((int64) rec.seed);
        static const uint32 sc[] { 0xffffffff, 0xff222222, 0xffe8342a, 0xffffa8d8 };
        for (int s = 0; s < 4; ++s)
            for (int k = 0; k < (int) (amt[(size_t) (salt + s)] * 14); ++k)
            {
                const float u = rr.nextFloat(), v = rr.nextFloat();
                const auto pt = Point<float> (dome.getX() + dome.getWidth() * (0.15f + 0.7f * u), dome.getBottom() - dome.getHeight() * (0.15f + 0.7f * v * (1.0f - std::abs (u - 0.5f) * 1.4f)));
                g.setColour (Colour (sc[s])); if (s == 2) g.fillEllipse (pt.x - 3, pt.y - 2, 6, 4); else if (s == 3) drawStar (g, pt, 4.0f, Colour (sc[s])); else g.fillEllipse (pt.x - 1.5f, pt.y - 1.5f, 3, 3);
            }
        // hot dishes steam, frozen ones sparkle
        if (ck.heat > 0.15f && plateDrop < 0.05f) for (int i = 0; i < 3; ++i)
        {
            Path s; const float x = dome.getCentreX() + (float) (i - 1) * 22.0f, y = dome.getY() - 6;
            s.startNewSubPath (x, y); for (int k = 1; k <= 8; ++k) s.lineTo (x + std::sin ((float) k * 0.9f + phase * 3.0f + (float) i) * 5.0f, y - (float) k * 5.0f);
            g.setColour (Colours::white.withAlpha (0.45f * ck.heat)); g.strokePath (s, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        if (fz > 0.2f) for (int i = 0; i < 5; ++i) drawStar (g, { dome.getX() + dome.getWidth() * (0.1f + 0.2f * (float) i), dome.getY() + dome.getHeight() * (0.3f + 0.15f * std::sin ((float) i * 2.0f + phase)) }, 5.0f, Colours::white.withAlpha (0.8f * fz));
        // the dish name + the recipe code
        const float ty = pr.getBottom() + 4;
        g.setColour (t.text); g.setFont (kk::modern::font (22.0f, true, 0.04f));
        g.drawFittedText (cur.valid() ? cur.name : String ("-"), Rectangle<float> (a.getX() + 14, ty, a.getWidth() - 28, 30).toNearestInt(), Justification::centred, 1, 0.65f);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.12f));
        const String what = useBase ? String ("your keys' sound, cooked") : String (kk::alc::exciters()[(size_t) ck.exc].name) + "  x  " + kk::alc::bodies()[(size_t) ck.body].name + "   -   " + kk::alc::matterWord (ck.matter).toUpperCase();
        g.drawText (what, Rectangle<float> (a.getX() + 14, ty + 30, a.getWidth() - 28, 16).toNearestInt(), Justification::centred);
        const auto cr = codeChip().toFloat();
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (cr, 8);
        g.setColour (kk::accentText()); g.drawRoundedRectangle (cr.reduced (0.5f), 8, 1.2f);
        g.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), useBase ? 12.0f : 18.0f, Font::bold)));
        g.setColour (useBase ? t.dim : Colour (0xffffd27a));
        g.drawFittedText (useBase ? String ("no code yet") : encode (rec), cr.reduced (64, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        g.setColour (t.dim); g.setFont (kk::modern::font (9.5f, true, 0.2f));
        g.drawText ("RECIPE", cr.reduced (10, 0).toNearestInt(), Justification::centredLeft);
        if (note.isNotEmpty())
        {
            g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.0f, true, 0.03f));
            g.drawFittedText (note, Rectangle<int> ((int) a.getX() + 14, copyBtn.getBottom() + 4, (int) a.getWidth() - 28, jmax (16, (int) a.getBottom() - copyBtn.getBottom() - 6)), Justification::centred, 2, 0.8f);
        }
    }
    void drawBook (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = bookArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("COOKBOOK", (int) a.getX() + 16, (int) a.getY() + 12, 130, 20, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.02f));
        g.drawFittedText (book.empty() ? String ("SERVE a dish - its recipe lands here") : String ("click = cook it again\nright-click = copy code / take out"), Rectangle<int> ((int) a.getX() + 16, (int) a.getY() + 36, 126, 60), Justification::topLeft, 3, 0.85f);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (a.withTrimmedLeft (146).reduced (0, 4).toNearestInt());
        for (int i = bookScroll; i < (int) book.size(); ++i)
        {
            const auto r = bookChip (i);
            if (r.getX() > a.getRight()) break;
            kk::code::Recipe rc; if (! kk::code::decode (book[(size_t) i], rc)) continue;
            const auto ck = kk::code::cook (rc);
            const auto ca = ck.main >= 0 ? Colour (kk::code::ingredients()[(size_t) ck.main].colour) : Colour (0xff6fb7d8), cb = ck.second >= 0 ? Colour (kk::code::ingredients()[(size_t) ck.second].colour) : ca;
            const bool same = rc == rec && ! useBase;
            g.setColour (Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (r, 10);
            g.setColour (same ? kk::accentText() : t.text.withAlpha (0.18f)); g.drawRoundedRectangle (r.reduced (0.5f), 10, same ? 2.0f : 1.0f);
            const auto dot = Rectangle<float> (r.getX() + 8, r.getY() + 10, 28, 28);
            g.setColour (Colours::white.withAlpha (0.85f)); g.fillEllipse (dot.expanded (3));
            g.setGradientFill (ColourGradient (ca, dot.getX(), dot.getY(), cb, dot.getRight(), dot.getBottom(), false)); g.fillEllipse (dot);
            g.setColour (t.text); g.setFont (kk::modern::font (11.0f, true, 0.04f));
            g.drawFittedText (ck.dish, Rectangle<float> (r.getX() + 42, r.getY() + 6, r.getWidth() - 48, 34).toNearestInt(), Justification::centredLeft, 2, 0.75f);
            g.setColour (t.dim); g.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), 9.5f, Font::plain)));
            g.drawFittedText (book[(size_t) i], Rectangle<float> (r.getX() + 6, r.getBottom() - 20, r.getWidth() - 12, 14).toNearestInt(), Justification::centred, 1, 0.6f);
        }
    }
    void drawCarried (Graphics& g)
    {
        if (mode != mJar || ! moved) return;
        int n = 0;
        for (int i = 0; i < kk::code::numIngredients; ++i)
            if (carry[(size_t) i])
            {
                const auto c = carryPos + Point<float> ((float) (n % 3) * 26.0f - 26.0f, (float) (n / 3) * 26.0f - 20.0f);
                g.setColour (Colours::black.withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (40, 40).withCentre (c).translated (3, 4));
                g.setColour (Colour (kk::code::ingredients()[(size_t) i].colour).withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (40, 40).withCentre (c));
                drawIngredient (g, i, Rectangle<float> (32, 32).withCentre (c), 1.0f);
                ++n;
            }
        if (potRect().expanded (40).contains (carryPos)) { g.setColour (Colour (0xffffd27a)); g.drawRoundedRectangle (potRect().expanded (10), 18, 2.5f); }
    }

    void timerCallback() override
    {
        if (! isShowing()) return;
        using namespace kk::code;
        phase += 0.04f;
        const float dt = 1.0f / 30.0f;
        const auto now = Time::getMillisecondCounter();
        bool grew = false;
        auto grow = [&] (int a, float perSecond) { if (amt[(size_t) a] < 1.0f) { amt[(size_t) a] = jmin (1.0f, amt[(size_t) a] + perSecond * dt); grew = true; } };
        if (panOnFire) grow (fry, mode == mPan ? 0.16f : 0.07f);
        if (dishWhere == 1) grow (bake, 0.07f);
        if (dishWhere == 2) grow (freeze, 0.08f);
        if (mode == mFreeze) grow (freeze, 0.14f);
        if (mode == mBlend) grow (blend, 0.18f);
        if (mode == mPot && now - lastMoveMs > 300) grow (boil, 0.14f);
        if (grew) { sync(); live(); }
        if (! (mode == mPot && moved)) swirl += 0.01f + 0.05f * amt[stir];
        chopFlash = jmax (0.0f, chopFlash - 0.12f);
        plateDrop = jmax (0.0f, plateDrop - 0.08f);
        for (auto& b : particles) { b.pos += b.vel; if (b.kind == 5) b.vel.y += 0.5f; b.life -= b.kind == 4 ? 0.04f : 0.035f; }
        particles.erase (std::remove_if (particles.begin(), particles.end(), [] (const Bit& b) { return b.life <= 0; }), particles.end());
        if (note.isNotEmpty() && now - noteMs > 6000) note.clear();
        repaint();
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton serveBtn { lnf }, playBtn { lnf }, mutateBtn { lnf }, plantBtn { lnf }, saveBtn { lnf }, lifeBtn { lnf }, useBtn { lnf }, copyBtn { lnf }, enterBtn { lnf }, cleanBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    kk::code::Recipe rec, lastRec;
    std::array<float, kk::code::numActs> amt {};
    std::array<bool, kk::code::numIngredients> hand {}, carry {};
    KeysKillaProcessor::Genome cur, base;
    bool useBase = false, panOnFire = false, moved = false, phrase = false;
    int dishWhere = 0, downJar = -1, shakeIdx = 0, lastDir = 0, hoverJar = -1, bookScroll = 0;
    Mode mode = mNone;
    Point<float> downPos, lastPos, panPos, dishPos, carryPos;
    float lastAng = 0, swirl = 0, knifeX = 0, knifeY = 0.2f, strokeFrom = 0, shakeOff = 0, chopFlash = 0, plateDrop = 0, phase = 0;
    uint32 lastMoveMs = 0, lastLive = 0, noteMs = 0;
    std::vector<Bit> particles;
    std::vector<String> book;
    std::function<void()> onPicked;
    String pickTitle, note;
};
