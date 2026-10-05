// v0.45 GAME page (included by PluginEditor.cpp): the EVOLVE ARCADE.  Three light, vector-drawn games - SOUND INVADERS,
// PIXEL DUEL, PINBALL - where every hit is a note in the key played with the sound on your keys.  Clearing a level or
// beating a score opens a CHEST of 1-5 sounds (COMMON / RARE / EPIC / LEGENDARY); unlocked sounds land in MY SOUNDS > UNLOCKED
// and in the COLLECTION BOOK (250 recipes).  The page takes the keyboard only while a game runs (FL's keys work otherwise),
// and its 60 fps timer only runs while it is showing.  The game logic lives in Games.h (kk::game).
#include "Games.h"

// the arcade's drag buttons always sit on dark panels: white text in both themes
class ArcadeDragButton : public DragFileButton
{
public:
    ArcadeDragButton (String t, Colour c) : DragFileButton (t, c), txt (t), col (c) {}
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1);
        g.setColour (col.withAlpha (isMouseOver() ? 0.34f : 0.2f)); g.fillRoundedRectangle (r, 8);
        g.setColour (col); g.drawRoundedRectangle (r, 8, 1.6f);
        for (int k = 0; k < 3; ++k) g.fillRect (r.getX() + 12, r.getCentreY() - 6 + (float) k * 5.0f, 14.0f, 2.0f);
        g.setColour (Colours::white); g.setFont (kk::modern::font (14.0f, true, 0.12f));
        g.drawText (txt, r.withTrimmedLeft (30), Justification::centred);
    }
    void mouseEnter (const MouseEvent& e) override { DragFileButton::mouseEnter (e); repaint(); }
    void mouseExit (const MouseEvent& e) override { DragFileButton::mouseExit (e); repaint(); }
private:
    String txt; Colour col;
};

class GamePage : public Component, private Timer
{
public:
    enum View { vMenu, vInvaders, vDuel, vPinball, vChest, vBook };
    using Recipe = kk::game::Recipe;
    GamePage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        setWantsKeyboardFocus (false); setMouseClickGrabsKeyboardFocus (false);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (menuTab, "ARCADE", "The three games", [this] { show (vMenu); });
        btn (bookTab, "COLLECTION", "The COLLECTION BOOK: 250 sounds to unlock - which ones you have, how rare, how many times", [this] { show (vBook); });
        btn (againBtn, "PLAY AGAIN", "Another run", [this] { startGame (chest.game, chestWin && chest.game != kk::game::gPinball ? nextLevel() : lastLevel); });
        btn (arcadeBtn, "ARCADE", "Back to the arcade", [this] { show (vMenu); });
        btn (openBtn, "OPEN THE CHEST", "Open it!", [this] { openTheChest(); });
        btn (bookPlayBtn, "PLAY IT", "Hear this sound (it goes on your keys)", [this] { playSlot (bookSel); });
        btn (backBtn, "BACK (ESC)", "Leave the game - back to the arcade", [this] { show (vMenu); });
        openBtn.hero = true; againBtn.hero = true;
        dragWav.makeFile = [this] { auto* c = selCard(); if (c == nullptr) return File(); ensureGenome (*c); return c->g.valid() ? proc.exportGenomeWav (c->g) : File(); };
        dragWav.setTooltip ("Drag the selected sound into FL as a WAV");
        dragMidi.makeFile = [this] { return exportMidi(); };
        dragMidi.setTooltip ("The pinball's bounces as a melody - drop the MIDI on any instrument in FL");
        bookWav.makeFile = [this] { if (bookSel < 0 || coll.count[(size_t) bookSel] <= 0) return File(); auto g = kk::game::genomeFor (proc, kk::game::recipeForSlot (bookSel)); return g.valid() ? proc.exportGenomeWav (g) : File(); };
        bookWav.setTooltip ("Drag this sound into FL as a WAV");
        for (auto* d : { &dragWav, &dragMidi, &bookWav }) addAndMakeVisible (*d);
        loadState();
        show (vMenu);
    }
    ~GamePage() override { stopTimer(); releaseAll(); }

    // snapshots / tests: 0 arcade, 1 invaders mid-game, 2 duel mid-fight, 3 pinball mid-run, 4 an opened legendary chest, 5 the book
    void debugShow (int which)
    {
        persist = false;
        Random r (7);
        if (which == 1)
        {
            startGame (kk::game::gInvaders, 1);
            for (int i = 0; i < 60 * 9 && ! inv.over; ++i) { const float tt = (float) i / 60.0f; handle (inv.step (1.0f / 60.0f, std::sin (tt * 1.3f) < -0.3f, std::sin (tt * 1.3f) > 0.3f, true), kk::game::gInvaders, false); updateSparks (1.0f / 60.0f); }
            inv.lives = std::max (inv.lives, 2);
        }
        else if (which == 2)
        {
            startGame (kk::game::gDuel, 2);
            for (int i = 0; i < 60 * 7 && duel.winner < 0; ++i) { auto a = duel.cpu (0, 1.0f / 60.0f), b = duel.cpu (1, 1.0f / 60.0f); handle (duel.step (1.0f / 60.0f, a, b), kk::game::gDuel, false); if (duel.f[0].hp < 30 || duel.f[1].hp < 30) break; }
            duel.f[0].act = kk::game::Duel::aPunch; duel.f[0].actT = 0.1f; duel.f[0].charge = 1.0f;
            if (duel.waves.empty()) duel.waves.push_back ({ duel.f[1].x + 0.12f * (float) duel.f[1].facing, 0.06f, (float) duel.f[1].facing * 0.85f, 1, true });
        }
        else if (which == 3)
        {
            startGame (kk::game::gPinball, 1);
            bool lf = false, rf = false;
            for (int i = 0; i < 60 * 30 && pin.balls == 3; ++i)
            {
                if (i % 20 == 0) { lf = r.nextFloat() < 0.4f; rf = r.nextFloat() < 0.4f; }
                const bool plunge = pin.inLane && (i % 90) < 50;
                handle (pin.step (1.0f / 60.0f, lf, rf, plunge), kk::game::gPinball, false); updateTrail();
                if (pin.hitsTotal > 14 && ! pin.inLane && pin.by < 0.9f) break;
            }
        }
        else if (which == 4)
        {
            auto c = kk::game::openChest (kk::game::gInvaders, 3, 2600, true);
            if (c.best() < kk::game::rLegendary) c.items.insert (c.items.begin(), kk::game::recipeForSlot (kk::game::slotStart (kk::game::rLegendary) + 4));
            while (c.items.size() > 5) c.items.pop_back();
            chest = c; chestWin = true; chestTitle = "LEVEL CLEAR"; buildCards (nullptr);
            show (vChest); openTheChest(); selected = 0; openT = 1.0f;
        }
        else if (which == 5)
        {
            for (int i = 0; i < kk::game::numSlots; ++i) { Random q ((int64) kk::hash32 ((uint32) i * 977u + 5u)); if (q.nextFloat() < (i < 130 ? 0.55f : i < 200 ? 0.35f : i < 235 ? 0.2f : 0.15f)) coll.count[(size_t) i] = 1 + q.nextInt (i < 130 ? 5 : 2); }
            show (vBook); bookSel = kk::game::slotStart (kk::game::rLegendary) + 2; coll.count[(size_t) bookSel] = std::max (1, coll.count[(size_t) bookSel]);
        }
        else show (vMenu);
        paused = false;
        refreshButtons(); resized(); repaint();
    }

    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("ARCADE", 24, 12, 170, 40, Justification::centredLeft);
        static const char* subs[] { "play - every hit is a note in the key, played with the sound on your keys.  win a level or beat a score = a CHEST of new sounds",
                                    "SOUND INVADERS  -  left / right (or A / D or the mouse) move,  SPACE (or click) shoots.  every 3rd wave: the 808 BOSS",
                                    "PIXEL DUEL  -  left / right move,  UP jumps,  Z (or click) punches,  X (or right-click) fires the special.  win = steal the rival's sound",
                                    "PINBALL  -  hold SPACE (or the lane) = plunger,  Z / M (or left / right mouse) = flippers.  bumpers are notes and ingredients",
                                    "your chest - click a sound to hear it on your keys.  new sounds are saved into MY SOUNDS > UNLOCKED by themselves",
                                    "the COLLECTION BOOK - 250 sounds, rarer = harder to get.  click one you have to hear it" };
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawFittedText (subs[view], 190, 18, getWidth() - 190 - 300, 28, Justification::centredLeft, 1, 0.8f);
        const auto st = stage().toFloat();
        switch (view)
        {
            case vInvaders: drawInvaders (g, st); break;
            case vDuel:     drawDuel (g, st); break;
            case vPinball:  drawPinball (g, st); break;
            case vChest:    drawChest (g, st); break;
            case vBook:     drawBook (g, st); break;
            default:        drawMenu (g, st); break;
        }
        if (isGame() && paused)
        {
            g.setColour (Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (st, 18);
            g.setColour (Colours::white); g.setFont (kk::modern::font (40.0f, true, 0.2f));
            g.drawText ("PAUSED", st.withTrimmedBottom (st.getHeight() * 0.1f).toNearestInt(), Justification::centred);
            g.setColour (Colours::white.withAlpha (0.7f)); g.setFont (kk::modern::font (15.0f, true, 0.1f));
            g.drawText ("click the game to play  -  your keys are FL's again while it is paused", st.withTrimmedTop (st.getHeight() * 0.12f).toNearestInt(), Justification::centred);
        }
    }
    void resized() override
    {
        menuTab.setBounds (getWidth() - 290, 14, 120, 34); bookTab.setBounds (getWidth() - 162, 14, 140, 34);
        const auto st = stage();
        backBtn.setBounds (st.getX() + 14, st.getBottom() - 50, 150, 38);
        // the chest view
        const int by = st.getBottom() - 56;
        openBtn.setBounds (st.getCentreX() - 130, st.getY() + 290, 260, 50);
        int x = st.getCentreX() - (190 * 4 + 30) / 2;
        dragWav.setBounds (x, by, 190, 44); dragMidi.setBounds (x + 200, by, 190, 44); againBtn.setBounds (x + 400, by, 180, 44); arcadeBtn.setBounds (x + 590, by, 160, 44);
        // the pinball's own DRAG MIDI sits in its right panel while playing
        if (view == vPinball) { const auto tr = pinTable(); dragMidi.setBounds ((int) tr.getRight() + 30, (int) tr.getBottom() - 60, std::min (240, st.getRight() - (int) tr.getRight() - 50), 44); }
        const auto bp = bookPanel();
        bookPlayBtn.setBounds (bp.getX(), bp.getY() + 236, 150, 42); bookWav.setBounds (bp.getX() + 160, bp.getY() + 235, 170, 44);
    }

    // ---------------------------------------------------------------- input ----------------------------------------------------------------
    bool keyPressed (const KeyPress& k) override
    {
        if (! isGame()) return false;
        const int c = k.getKeyCode();
        if (c == KeyPress::escapeKey) { show (vMenu); return true; }
        if (c == 'p' || c == 'P') { paused = ! paused; releaseAll(); return true; }
        if (c == KeyPress::spaceKey || c == KeyPress::upKey || c == 'w' || c == 'W') tapA = true;
        if (c == 'z' || c == 'Z' || c == 'j' || c == 'J') tapB = true;
        if (c == 'x' || c == 'X' || c == 'k' || c == 'K') tapC = true;
        return true;   // the game owns the keys while it runs (no FL shortcuts fire by accident)
    }
    bool keyStateChanged (bool) override { return isGame(); }
    void focusLost (FocusChangeType) override { if (isGame()) { paused = true; releaseAll(); repaint(); } }
    void mouseDown (const MouseEvent& e) override
    {
        const auto pos = e.position;
        if (view == vMenu)
        {
            for (int i = 0; i < 3; ++i) if (cabinet (i).contains (e.getPosition())) { startGame (i, i == kk::game::gInvaders ? invLevel : i == kk::game::gDuel ? duelLevel : 1); return; }
            if (menuStrip().contains (e.getPosition())) show (vBook);
            return;
        }
        if (view == vChest)
        {
            if (! opened && chestBox().expanded (20).contains (pos) && ! cards.empty()) { openTheChest(); return; }
            for (int i = 0; i < (int) cards.size(); ++i) if (cardRect (i).contains (pos)) { selected = i; playCard (i); return; }
            return;
        }
        if (view == vBook)
        {
            const int s = slotAt (pos);
            if (s >= 0) { bookSel = s; if (coll.count[(size_t) s] > 0) playSlot (s); refreshButtons(); repaint(); }
            return;
        }
        if (isGame())
        {
            if (paused) { paused = false; grabFocus(); lastMs = Time::getMillisecondCounterHiRes(); repaint(); return; }
            if (! hasKeyboardFocus (true)) grabFocus();
            const bool right = e.mods.isPopupMenu();
            if (view == vPinball)
            {
                if (pinLane().contains (pos)) mousePlunger = true;
                else if (right) mouseR = true; else mouseL = true;
            }
            else if (view == vInvaders) { mouseL = true; mouseX = toField (pos).x; mouseCtl = true; }
            else if (view == vDuel) { if (right) tapC = true; else tapB = true; }
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) mouseR = false; else mouseL = false;
        mousePlunger = false;
    }
    void mouseDrag (const MouseEvent& e) override { if (view == vInvaders) { mouseX = toField (e.position).x; mouseCtl = true; } }
    void mouseMove (const MouseEvent& e) override
    {
        if (view == vInvaders && fieldRect().contains (e.position)) { const float nx = toField (e.position).x; if (std::abs (nx - mouseX) > 0.002f) { mouseX = nx; mouseCtl = true; } }
        if (view == vMenu) { int h = -1; for (int i = 0; i < 3; ++i) if (cabinet (i).contains (e.getPosition())) h = i; hover = h; }
        if (view == vBook) { const int s = slotAt (e.position); if (s != bookHover) { bookHover = s; repaint(); } }
        if (view == vChest) { int h = -1; for (int i = 0; i < (int) cards.size(); ++i) if (cardRect (i).contains (e.position)) h = i; hover = h; }
    }
    void mouseExit (const MouseEvent&) override { hover = -1; bookHover = -1; }
    void visibilityChanged() override
    {
        if (isVisible()) { lastMs = Time::getMillisecondCounterHiRes(); startTimerHz (isGame() ? 60 : 30); }
        else { stopTimer(); releaseAll(); dropFocus(); if (isGame()) paused = true; }
    }

    // ---------------------------------------------------------------- state ----------------------------------------------------------------
    kk::game::Collection coll;
private:
    struct Card { Recipe r; KeysKillaProcessor::Genome g; bool isNew = false, saved = false; int count = 0; };
    struct Spark { float x, y, vx, vy, life; Colour c; String txt; int game; };
    struct Held { int pitch; double off; };

    bool isGame() const { return view == vInvaders || view == vDuel || view == vPinball; }
    Rectangle<int> stage() const { return { 24, 72, getWidth() - 48, getHeight() - 96 }; }
    kk::game::Key key() const { kk::game::Key k; k.root = ((proc.melKey % 12) + 12) % 12; k.minor = kk::mel::isMinorish (proc.melScale); return k; }

    void show (int v)
    {
        if (isGame() && v != view) releaseAll();
        view = v; hover = -1;
        if (isGame()) { grabFocus(); paused = false; } else dropFocus();
        lastMs = Time::getMillisecondCounterHiRes();
        if (isVisible()) startTimerHz (isGame() ? 60 : 30);
        refreshButtons(); resized(); repaint();
    }
    void refreshButtons()
    {
        menuTab.selected = view == vMenu || isGame(); bookTab.selected = view == vBook;
        const bool ch = view == vChest;
        openBtn.setVisible (ch && ! opened && ! cards.empty());
        dragWav.setVisible (ch && opened && ! cards.empty());
        dragMidi.setVisible ((ch && chest.game == kk::game::gPinball && ! lastMelody.notes.empty()) || (view == vPinball && ! pin.notes.empty()));
        againBtn.setVisible (ch); arcadeBtn.setVisible (ch);
        againBtn.setButtonText (chestWin && chest.game != kk::game::gPinball ? "NEXT LEVEL" : "PLAY AGAIN");
        backBtn.setVisible (isGame());
        bookPlayBtn.setVisible (view == vBook && bookSel >= 0 && coll.count[(size_t) bookSel] > 0);
        bookWav.setVisible (bookPlayBtn.isVisible());
        menuTab.repaint(); bookTab.repaint();
    }
    void grabFocus() { setWantsKeyboardFocus (true); if (isShowing()) grabKeyboardFocus(); }
    void dropFocus() { if (hasKeyboardFocus (true)) giveAwayKeyboardFocus(); setWantsKeyboardFocus (false); }

    void startGame (int game, int level)
    {
        releaseAll(); sparks.clear(); trail.clear(); shake = 0; tapA = tapB = tapC = false; mouseL = mouseR = mousePlunger = false;
        lastLevel = level; runSeed = (uint32) Random::getSystemRandom().nextInt() | 1u;
        if (game == kk::game::gInvaders) { inv.start (runSeed, key()); inv.wave = (std::max (1, level) - 1) * 3 + 1; inv.spawnWave(); show (vInvaders); }
        else if (game == kk::game::gDuel)
        {
            duel.start (level, runSeed, key()); rival = kk::game::rivalRecipe (level);
            const auto g = proc.currentGenome(); mySound = g.valid() && g.name.isNotEmpty() ? g.name : String ("your sound");
            show (vDuel);
        }
        else { pin.start (runSeed, key()); show (vPinball); }
    }
    int nextLevel() const { return chest.game == kk::game::gDuel ? duelLevel : invLevel; }

    // ---------------------------------------------------------------- sound: notes on the keys ----------------------------------------------------------------
    void hear (int pitch, float vel, double len = 0.22)
    {
        if (pitch < 0 || pitch > 127) return;
        const double now = Time::getMillisecondCounterHiRes();
        for (auto it = held.begin(); it != held.end(); ++it) if (it->pitch == pitch) { proc.keyboardState.noteOff (1, pitch, 0.0f); held.erase (it); break; }
        while (held.size() >= 6) { proc.keyboardState.noteOff (1, held.front().pitch, 0.0f); held.erase (held.begin()); }
        proc.keyboardState.noteOn (1, pitch, jlimit (0.05f, 1.0f, vel));
        held.push_back ({ pitch, now + len * 1000.0 });
    }
    void releaseDue (double now)
    {
        for (size_t i = 0; i < held.size();)
            if (held[i].off <= now) { proc.keyboardState.noteOff (1, held[i].pitch, 0.0f); held.erase (held.begin() + (long) i); } else ++i;
    }
    void releaseAll() { for (auto& h : held) proc.keyboardState.noteOff (1, h.pitch, 0.0f); held.clear(); }

    // ---------------------------------------------------------------- the loop ----------------------------------------------------------------
    static bool down (std::initializer_list<int> codes) { for (auto c : codes) if (KeyPress::isKeyCurrentlyDown (c)) return true; return false; }
    void timerCallback() override
    {
        // hidden (another page, the window closed): no game, no keys, no focus - only a slow check until it shows again
        if (! isShowing()) { if (getTimerInterval() != 250) { startTimer (250); releaseAll(); dropFocus(); if (isGame()) paused = true; } return; }
        if (getTimerInterval() == 250) { startTimerHz (isGame() ? 60 : 30); lastMs = Time::getMillisecondCounterHiRes(); }
        const double now = Time::getMillisecondCounterHiRes();
        const float dt = (float) jlimit (0.0, 0.05, (now - lastMs) / 1000.0); lastMs = now;
        phase += dt;
        releaseDue (now);
        if (isGame() && ! paused)
        {
            acc += dt;
            const float h = 1.0f / 120.0f;
            while (acc >= h) { stepGame (h); acc -= h; if (! isGame()) break; }
        }
        if (isGame()) updateSparks (dt);
        if (view == vPinball && ! dragMidi.isVisible() && ! pin.notes.empty()) { refreshButtons(); resized(); }
        shake = std::max (0.0f, shake - dt * 3.0f);
        if (view == vChest) processSaveQueue();
        repaint();
    }
    void stepGame (float h)
    {
        const bool kf = hasKeyboardFocus (true);
        const bool kl = kf && down ({ KeyPress::leftKey, 'a', 'A' }), kr = kf && down ({ KeyPress::rightKey, 'd', 'D' });
        if (view == vInvaders)
        {
            bool l = kl, r = kr;
            if (kl || kr) mouseCtl = false;
            if (mouseCtl && ! kl && ! kr) { l = mouseX < inv.px - 0.008f; r = mouseX > inv.px + 0.008f; }
            const bool fire = tapA || mouseL || (kf && down ({ KeyPress::spaceKey, KeyPress::upKey, 'w', 'W' }));
            tapA = false;
            handle (inv.step (h, l, r, fire), kk::game::gInvaders, true);
            if (inv.over) finishInvaders();
        }
        else if (view == vDuel)
        {
            kk::game::Duel::Input in;
            in.left = kl; in.right = kr;
            in.jump = tapA || (kf && down ({ KeyPress::upKey, 'w', 'W', KeyPress::spaceKey }));
            in.punch = tapB || (kf && down ({ 'z', 'Z', 'j', 'J' }));
            in.special = tapC || (kf && down ({ 'x', 'X', 'k', 'K' }));
            if (in.punch || in.special || in.jump) tapA = tapB = tapC = false;
            handle (duel.step (h, in, duel.cpu (1, h)), kk::game::gDuel, true);
            if (duel.winner >= 0) finishDuel();
        }
        else if (view == vPinball)
        {
            const bool lf = mouseL || (kf && down ({ 'z', 'Z', KeyPress::leftKey })), rf = mouseR || (kf && down ({ 'm', 'M', KeyPress::rightKey }));
            const bool pl = mousePlunger || (kf && down ({ KeyPress::spaceKey, KeyPress::downKey }));
            handle (pin.step (h, lf, rf, pl), kk::game::gPinball, true);
            updateTrail();
            if (pin.over) finishPinball();
        }
    }
    void handle (const std::vector<kk::game::Ev>& evs, int game, bool sound)
    {
        for (auto& e : evs)
        {
            if (sound && e.pitch >= 0) hear (e.pitch, e.vel, game == kk::game::gPinball ? 0.3 : 0.2);
            Colour c = Colours::white; String txt = e.pitch >= 0 ? String (kk::mel::keyName (e.pitch % 12)) : String();
            int n = 0; float spd = 0.25f;
            if (game == kk::game::gInvaders)
            {
                using I = kk::game::Invaders;
                if (e.kind == I::evKill) { c = rowColour ((int) std::round ((e.y - 0.12f) / 0.07f)); n = 10; }
                else if (e.kind == I::evBossHit) { c = Colour (0xffff3b5c); n = 4; }
                else if (e.kind == I::evBossKill) { c = Colour (0xffffc63a); n = 40; spd = 0.6f; shake = 1.0f; if (sound) { hear (key().pitch (0, 0.0f), 1.0f, 0.6); hear (key().pitch (2, 0.0f), 0.9f, 0.6); } }
                else if (e.kind == I::evPlayerHit) { c = Colour (0xffff5a1f); n = 20; shake = 0.7f; }
                else continue;
            }
            else if (game == kk::game::gDuel)
            {
                using D = kk::game::Duel;
                if (e.kind == D::evHit || e.kind == D::evSpecialHit) { c = e.ref == 0 ? Colour (0xff36d6ff) : Colour (kk::game::rivalColour (duel.level)); n = e.kind == D::evSpecialHit ? 22 : 9; shake = e.kind == D::evSpecialHit ? 0.8f : 0.3f; txt = e.kind == D::evSpecialHit ? String ("SOUND WAVE!") : String(); }
                else if (e.kind == D::evKo) { c = Colour (0xffffc63a); n = 40; shake = 1.0f; txt = "K.O."; }
                else continue;
            }
            else
            {
                using P = kk::game::Pinball;
                if (e.kind == P::evBumper) { c = Colour (kk::game::ingredient (pin.bumpers[(size_t) e.ref].ing).colour); n = 8; }
                else if (e.kind == P::evRamp) { c = Colour (kk::game::ingredient (pin.ramps[(size_t) e.ref].ing).colour); n = 14; }
                else if (e.kind == P::evSling) { c = Colour (kk::game::ingredient (e.ref).colour); n = 5; }
                else if (e.kind == P::evDrain) { c = Colour (0xffff3b5c); n = 0; shake = 0.5f; txt = "DRAIN"; }
                else continue;
            }
            Random rr ((int64) (e.x * 10000) + (int64) sparks.size());
            for (int i = 0; i < n && sparks.size() < 260; ++i) { const float a = rr.nextFloat() * MathConstants<float>::twoPi, s = spd * (0.3f + rr.nextFloat()); sparks.push_back ({ e.x, e.y, std::cos (a) * s, std::sin (a) * s, 1.0f, c, {}, game }); }
            if (txt.isNotEmpty() && sparks.size() < 280) sparks.push_back ({ e.x, e.y, 0.0f, -0.12f, 1.4f, c, txt, game });
        }
    }
    void updateSparks (float dt)
    {
        for (auto& s : sparks) { s.x += s.vx * dt; s.y += s.vy * dt; s.vx *= 0.96f; s.vy *= 0.96f; s.life -= dt * (s.txt.isEmpty() ? 1.6f : 0.9f); }
        sparks.erase (std::remove_if (sparks.begin(), sparks.end(), [] (const Spark& s) { return s.life <= 0; }), sparks.end());
    }
    void updateTrail() { trail.push_back ({ pin.bx, pin.by }); while (trail.size() > 10) trail.erase (trail.begin()); }

    // ---------------------------------------------------------------- the end of a run: the chest ----------------------------------------------------------------
    void finishInvaders()
    {
        releaseAll();
        const int lvl = inv.level(); chestWin = inv.victory;
        if (chestWin) { invLevel = std::max (invLevel, lvl + 1); }
        best[0] = std::max (best[0], inv.score);
        chestTitle = chestWin ? "THE 808 BOSS IS DOWN  -  LEVEL " + String (lvl) + " CLEAR" : "GAME OVER  -  WAVE " + String (inv.wave);
        endRun (kk::game::gInvaders, lvl, inv.score, chestWin, {});
    }
    void finishDuel()
    {
        releaseAll();
        const bool won = duel.winner == 0; chestWin = won;
        const int score = (int) (duel.f[0].hits * 25 + duel.f[0].hp * 4 + duel.timeLeft * 3) + (won ? 200 * duel.level : 0);
        best[1] = std::max (best[1], score);
        std::vector<Recipe> extra;
        if (won) { extra.push_back (rival); duelLevel = std::max (duelLevel, duel.level + 1); }
        chestTitle = won ? "YOU WIN  -  " + String (kk::game::rivalName (duel.level)) + "'S SOUND IS YOURS" : String (kk::game::rivalName (duel.level)) + " WINS  -  TRY AGAIN";
        endRun (kk::game::gDuel, duel.level, score, won, extra);
    }
    void finishPinball()
    {
        releaseAll();
        lastMelody = pin.melody();
        best[2] = std::max (best[2], pin.score);
        std::vector<Recipe> extra;
        if (pin.hitsTotal > 0) extra.push_back (kk::game::dishRecipe (pin.dish(), pin.score));
        chestWin = false;
        chestTitle = "THE RUN IS COOKED  -  " + String (pin.score) + " POINTS";
        endRun (kk::game::gPinball, 1, pin.score, false, extra);
    }
    void endRun (int game, int level, int score, bool levelDone, const std::vector<Recipe>& extra)
    {
        if (kk::game::earnsChest (game, score, levelDone)) chest = kk::game::openChest (game, level, score, levelDone);
        else { chest = {}; chest.game = game; chest.level = level; chest.score = score; }
        for (auto it = extra.rbegin(); it != extra.rend(); ++it) chest.items.insert (chest.items.begin(), *it);
        while (chest.items.size() > 6) chest.items.pop_back();
        buildCards (nullptr);
        saveState();
        show (vChest);
    }
    void buildCards (const void*)
    {
        cards.clear(); opened = false; selected = -1; saveQueue.clear(); savedNote.clear(); openT = 0;
        for (auto& r : chest.items) { Card c; c.r = r; cards.push_back (c); }
    }
    void openTheChest()
    {
        if (opened || cards.empty()) return;
        opened = true; openT = 0; selected = 0;
        for (int i = 0; i < (int) cards.size(); ++i)
        {
            auto& c = cards[(size_t) i];
            if (c.r.slot >= 0) { c.count = persist ? coll.add (c.r.slot) : std::max (1, coll.count[(size_t) c.r.slot]); c.isNew = c.count == 1; }
            else { c.isNew = true; c.count = 1; }
            if (c.isNew && persist) saveQueue.push_back (i);
        }
        if (persist) saveState();
        hear (key().pitch (0), 0.8f, 0.4); hear (key().pitch (2), 0.7f, 0.4); hear (key().pitch (4), 0.7f, 0.5);
        refreshButtons(); repaint();
    }
    void ensureGenome (Card& c) { if (! c.g.valid()) c.g = kk::game::genomeFor (proc, c.r); }
    Card* selCard() { return selected >= 0 && selected < (int) cards.size() && opened ? &cards[(size_t) selected] : nullptr; }
    void playCard (int i)
    {
        if (! opened || i < 0 || i >= (int) cards.size()) return;
        auto& c = cards[(size_t) i]; ensureGenome (c);
        if (c.g.valid()) proc.alcUse (c.g, true);
        repaint();
    }
    void playSlot (int s)
    {
        if (s < 0 || s >= kk::game::numSlots || coll.count[(size_t) s] <= 0) return;
        auto g = kk::game::genomeFor (proc, kk::game::recipeForSlot (s));
        if (g.valid()) proc.alcUse (g, true);
    }
    // one sound per tick (never during a game): render -> MY SOUNDS > UNLOCKED
    void processSaveQueue()
    {
        if (saveQueue.empty() || ! persist) return;
        const int i = saveQueue.front(); saveQueue.erase (saveQueue.begin());
        if (i < 0 || i >= (int) cards.size()) return;
        auto& c = cards[(size_t) i]; ensureGenome (c);
        if (! c.g.valid()) return;
        if (! kk::Library::folder ("UNLOCKED").getChildFile (File::createLegalFileName (c.r.name) + ".wav").existsAsFile())
        {
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (c.g, rate, 2.5), rate, rate, c.r.name);
            const auto keep = proc.lastFolder;
            proc.saveToFolder (snd, "UNLOCKED");
            proc.lastFolder = keep;
        }
        c.saved = true;
        int n = 0; for (auto& x : cards) n += x.saved;
        savedNote = String (n) + (n == 1 ? " new sound" : " new sounds") + " saved into MY SOUNDS > UNLOCKED";
    }
    File exportMidi()
    {
        const auto& m = view == vPinball ? pin.melody() : lastMelody;
        if (m.notes.empty()) return {};
        auto mf = kk::live::toMidi (m, 120.0);
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("EVOLVE Arcade"); dir.createDirectory();
        auto f = dir.getChildFile ("Pinball melody - " + String (kk::mel::keyName (key().root)) + " " + kk::mel::scaleName (key().scale()) + " - 120BPM.mid");
        f.deleteFile(); if (FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
        return f;
    }
    void loadState()
    {
        if (auto s = openSettings())
        {
            coll.fromString (s->getValue ("gameCollection"));
            auto b = StringArray::fromTokens (s->getValue ("gameBest", "0,0,0"), ",", "");
            for (int i = 0; i < 3 && i < b.size(); ++i) best[(size_t) i] = b[i].getIntValue();
            invLevel = jlimit (1, 99, s->getIntValue ("gameInvLevel", 1)); duelLevel = jlimit (1, 99, s->getIntValue ("gameDuelLevel", 1));
        }
    }
    void saveState()
    {
        if (! persist) return;
        if (auto s = openSettings())
        {
            s->setValue ("gameCollection", coll.toString());
            s->setValue ("gameBest", String (best[0]) + "," + String (best[1]) + "," + String (best[2]));
            s->setValue ("gameInvLevel", invLevel); s->setValue ("gameDuelLevel", duelLevel);
        }
    }

    // ---------------------------------------------------------------- drawing helpers ----------------------------------------------------------------
    static Colour rowColour (int row) { static const uint32 c[] { 0xffff4fd8, 0xffa06bff, 0xff3fa9ff, 0xff36ff6a, 0xffffd23f }; return Colour (c[jlimit (0, 4, row)]); }
    static void arenaBack (Graphics& g, Rectangle<float> r, Colour tint)
    {
        g.setGradientFill (ColourGradient (Colour (0xff070a14), r.getX(), r.getY(), Colour (0xff0c0618).interpolatedWith (tint, 0.08f), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 18);
        g.setColour (tint.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (0.5f), 18, 1.4f);
    }
    static void pixels (Graphics& g, const uint8* rows, int nRows, float x, float y, float px, Colour c)
    {
        g.setColour (c);
        for (int r = 0; r < nRows; ++r) for (int b = 0; b < 8; ++b) if (rows[r] & (0x80 >> b)) g.fillRect (x + (float) b * px, y + (float) r * px, px - 0.6f, px - 0.6f);
    }
    static void label (Graphics& g, const String& s, Rectangle<float> r, float size, Colour c, Justification j = Justification::centredLeft, float kern = 0.12f)
    {
        g.setColour (c); g.setFont (kk::modern::font (size, true, kern)); g.drawFittedText (s, r.toNearestInt(), j, 1, 0.75f);
    }
    void drawSparks (Graphics& g, int game, std::function<Point<float> (float, float)> map)
    {
        for (auto& s : sparks)
        {
            if (s.game != game) continue;
            const auto p = map (s.x, s.y); const float a = jlimit (0.0f, 1.0f, s.life);
            if (s.txt.isNotEmpty()) { g.setColour (s.c.withAlpha (a)); g.setFont (kk::modern::font (13.0f, true, 0.15f)); g.drawText (s.txt, Rectangle<float> (p.x - 80, p.y - 10, 160, 20), Justification::centred); }
            else { g.setColour (s.c.withAlpha (a * 0.9f)); g.fillRect (p.x - 2.0f, p.y - 2.0f, 4.0f, 4.0f); }
        }
    }
    static void sidePanel (Graphics& g, Rectangle<float> r, Colour tint)
    {
        g.setGradientFill (ColourGradient (Colour (0xff0a0d18), r.getX(), r.getY(), Colour (0xff100a1a), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 16);
        g.setColour (tint.withAlpha (0.25f)); g.drawRoundedRectangle (r.reduced (0.5f), 16, 1.0f);
    }
    void hudBlock (Graphics& g, Rectangle<float> r, const String& head, const String& value, Colour c)
    {
        g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 10);
        g.setColour (c.withAlpha (0.4f)); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1.0f);
        label (g, head, r.reduced (12, 6).withHeight (16), 11.5f, Colours::white.withAlpha (0.55f), Justification::centredLeft, 0.3f);
        label (g, value, r.reduced (12, 6).withTrimmedTop (16), 26.0f, c, Justification::centredLeft, 0.05f);
    }

    // ---------------------------------------------------------------- MENU ----------------------------------------------------------------
    Rectangle<int> menuStrip() const { const auto s = stage(); return s.withTop (s.getBottom() - 92); }
    Rectangle<int> cabinet (int i) const { const auto s = stage().withTrimmedBottom (110); const int w = (s.getWidth() - 2 * 24) / 3; return { s.getX() + i * (w + 24), s.getY(), w, s.getHeight() }; }
    static Colour gameColour (int i) { static const uint32 c[] { 0xff36ff6a, 0xffff4fd8, 0xffffb020 }; return Colour (c[jlimit (0, 2, i)]); }
    void drawMenu (Graphics& g, Rectangle<float>)
    {
        static const char* tag[] { "a retro shooter in your key", "pixel 1v1 against the CPU", "bumpers, ramps and a cooking pot" };
        static const char* how[][3] { { "every enemy you destroy = a note", "waves get faster, the 808 BOSS comes every 3rd", "clear the boss = a chest" },
                                      { "every hit sounds - yours high, the rival's low", "punch to charge your special sound wave", "win = STEAL the rival's sound" },
                                      { "every bumper is a note and an ingredient", "the bounces become a melody - DRAG MIDI", "the drained run COOKS a new sound" } };
        for (int i = 0; i < 3; ++i)
        {
            const auto r = cabinet (i).toFloat(); const auto c = gameColour (i); const bool hov = hover == i;
            g.setGradientFill (ColourGradient (Colour (0xff0a0d18), r.getX(), r.getY(), Colour (0xff120a1c), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 20);
            if (hov) { for (int k = 3; k >= 1; --k) { g.setColour (c.withAlpha (0.06f * (float) k)); g.drawRoundedRectangle (r.expanded ((float) k * 2.0f), 22, 3.0f); } }
            g.setColour (c.withAlpha (hov ? 0.95f : 0.5f)); g.drawRoundedRectangle (r.reduced (0.5f), 20, hov ? 2.2f : 1.4f);
            const auto scr = r.reduced (18).withHeight (r.getHeight() * 0.5f);
            g.setColour (Colour (0xff03050a)); g.fillRoundedRectangle (scr, 12);
            {
                Graphics::ScopedSaveState ss (g); g.reduceClipRegion (scr.toNearestInt());
                if (i == 0) previewInvaders (g, scr); else if (i == 1) previewDuel (g, scr); else previewPinball (g, scr);
                for (float y = scr.getY(); y < scr.getBottom(); y += 3.0f) { g.setColour (Colours::black.withAlpha (0.18f)); g.fillRect (scr.getX(), y, scr.getWidth(), 1.0f); }   // scanlines
            }
            g.setColour (c.withAlpha (0.35f)); g.drawRoundedRectangle (scr, 12, 1.0f);
            auto tx = r.reduced (24).withTop (scr.getBottom() + 14);
            label (g, kk::game::gameName (i), tx.removeFromTop (34), 27.0f, Colours::white, Justification::centredLeft, 0.1f);
            label (g, tag[i], tx.removeFromTop (22), 14.0f, c, Justification::centredLeft, 0.08f);
            tx.removeFromTop (8);
            for (int k = 0; k < 3; ++k)
            {
                auto row = tx.removeFromTop (22);
                g.setColour (c); g.fillEllipse (row.getX(), row.getCentreY() - 3, 6, 6);
                label (g, how[i][k], row.withTrimmedLeft (16), 13.0f, Colours::white.withAlpha (0.75f), Justification::centredLeft, 0.03f);
            }
            static const char* keys[] { "ARROWS / A D / MOUSE  move     SPACE / CLICK  shoot", "ARROWS  move + jump     Z  punch     X  special", "SPACE  plunger     Z / M  or  LEFT / RIGHT MOUSE  flippers" };
            tx.removeFromTop (12);
            const auto kr = tx.removeFromTop (30);
            g.setColour (c.withAlpha (0.1f)); g.fillRoundedRectangle (kr, 8);
            label (g, keys[i], kr.reduced (10, 0), 11.5f, c.withAlpha (0.9f), Justification::centredLeft, 0.1f);
            auto foot = r.reduced (24).removeFromBottom (44);
            const String lv = i == 0 ? "LEVEL " + String (invLevel) : i == 1 ? "RIVAL " + String (duelLevel) + "  " + kk::game::rivalName (duelLevel) : String ("3 BALLS");
            label (g, "BEST  " + String (best[(size_t) i]) + "     " + lv, foot.withTrimmedRight (130), 13.0f, Colours::white.withAlpha (0.6f), Justification::centredLeft, 0.15f);
            const auto pl = foot.removeFromRight (120).reduced (0, 4);
            g.setColour (c.withAlpha (hov ? 0.35f : 0.16f)); g.fillRoundedRectangle (pl, pl.getHeight() * 0.5f);
            g.setColour (c); g.drawRoundedRectangle (pl, pl.getHeight() * 0.5f, 1.5f);
            label (g, "PLAY", pl, 16.0f, Colours::white, Justification::centred, 0.25f);
        }
        // the collection strip
        const auto s = menuStrip().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff0a0d18), s.getX(), s.getY(), Colour (0xff160d06), s.getRight(), s.getBottom(), false)); g.fillRoundedRectangle (s, 16);
        g.setColour (Colour (0xffffc63a).withAlpha (0.35f)); g.drawRoundedRectangle (s.reduced (0.5f), 16, 1.2f);
        label (g, "COLLECTION BOOK", s.reduced (22, 0).withHeight (s.getHeight() * 0.5f).translated (0, 8), 15.0f, Colours::white, Justification::centredLeft, 0.25f);
        label (g, String (coll.unlocked()) + " / " + String (kk::game::numSlots) + " sounds unlocked  -  click to open", s.reduced (22, 0).withTrimmedTop (s.getHeight() * 0.5f).withHeight (24), 13.0f, Colours::white.withAlpha (0.6f), Justification::centredLeft, 0.04f);
        const float bx0 = s.getX() + 420, bw = (s.getRight() - 24 - bx0) / 4.0f;
        for (int r = 0; r < kk::game::numRarities; ++r)
        {
            const auto rc = Colour (kk::game::rarityColour (r));
            const int tot = kk::game::slotStart (r + 1) - kk::game::slotStart (r), have = coll.unlocked (r);
            const auto bar = Rectangle<float> (bx0 + (float) r * bw, s.getCentreY() + 4, bw - 24, 10);
            label (g, String (kk::game::rarityName (r)) + "  " + String (have) + " / " + String (tot), bar.withY (bar.getY() - 26).withHeight (20), 12.5f, rc, Justification::centredLeft, 0.2f);
            g.setColour (rc.withAlpha (0.15f)); g.fillRoundedRectangle (bar, 5);
            g.setColour (rc); g.fillRoundedRectangle (bar.withWidth (std::max (have > 0 ? 6.0f : 0.0f, bar.getWidth() * (float) have / (float) tot)), 5);
        }
    }
    void previewInvaders (Graphics& g, Rectangle<float> r)
    {
        static const uint8 sp[2][6] { { 0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24 }, { 0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x5a } };
        static const uint8 ship[4] { 0x18, 0x3c, 0x7e, 0xff };
        const float px = r.getWidth() / 110.0f, off = std::sin (phase * 0.8f) * r.getWidth() * 0.08f;
        for (int row = 0; row < 3; ++row) for (int col = 0; col < 6; ++col)
            if (! (row == 1 && col == ((int) (phase * 0.7f)) % 6)) pixels (g, sp[(int) (phase * 2) % 2], 6, r.getX() + r.getWidth() * 0.18f + (float) col * px * 13 + off, r.getY() + r.getHeight() * 0.12f + (float) row * px * 10, px, rowColour (row));
        const float sx = r.getCentreX() + std::sin (phase * 1.3f) * r.getWidth() * 0.3f;
        pixels (g, ship, 4, sx - px * 4, r.getBottom() - px * 9, px, Colour (0xff36ff6a));
        const float by = r.getBottom() - std::fmod (phase * r.getHeight() * 0.9f, r.getHeight() * 0.8f) - px * 10;
        g.setColour (Colours::white); g.fillRect (sx - 1.0f, by, 2.0f, px * 3);
    }
    void previewDuel (Graphics& g, Rectangle<float> r)
    {
        const float u = r.getHeight() / 50.0f, gy = r.getBottom() - u * 6;
        g.setColour (Colour (0xff2a1440)); g.fillRect (r.getX(), gy, r.getWidth(), u * 6);
        for (int i = 0; i < 9; ++i) { Random q (i + 4); const float w = u * (6 + q.nextInt (8)), hh = u * (8 + q.nextInt (20)); g.setColour (Colour (0xff1a0f2e)); g.fillRect (r.getX() + (float) i * r.getWidth() / 9.0f, gy - hh, w, hh); }
        const int poseA = std::fmod (phase, 1.6f) < 0.4f ? kk::game::Duel::aPunch : kk::game::Duel::aIdle;
        fighter (g, r.getCentreX() - u * 9 + std::sin (phase * 2) * u, gy, u * 0.9f, 1, poseA, Colour (0xff36d6ff), phase, false);
        fighter (g, r.getCentreX() + u * 9, gy, u * 0.9f, -1, poseA == kk::game::Duel::aPunch ? kk::game::Duel::aHurt : kk::game::Duel::aIdle, Colour (0xffff4fd8), phase, false);
    }
    void previewPinball (Graphics& g, Rectangle<float> r)
    {
        const float cx = r.getCentreX(), cy = r.getCentreY();
        static const float bp[][2] { { -0.18f, -0.15f }, { 0.12f, -0.22f }, { 0.0f, 0.08f }, { 0.22f, 0.05f } };
        for (int i = 0; i < 4; ++i)
        {
            const auto c = Colour (kk::game::ingredient (i).colour); const float x = cx + bp[i][0] * r.getWidth() * 0.9f, y = cy + bp[i][1] * r.getHeight() * 1.4f, rr = r.getHeight() * 0.09f;
            const float fl = std::max (0.0f, 1.0f - std::fmod (phase * 1.3f + (float) i * 0.7f, 2.8f));
            g.setColour (c.withAlpha (0.2f + 0.5f * fl)); g.fillEllipse (x - rr * 1.4f, y - rr * 1.4f, rr * 2.8f, rr * 2.8f);
            g.setColour (c); g.drawEllipse (x - rr, y - rr, rr * 2, rr * 2, 2.0f);
        }
        const float a = phase * 1.7f, bx = cx + std::cos (a) * r.getWidth() * 0.22f, by = cy + std::sin (a * 1.3f) * r.getHeight() * 0.3f;
        g.setColour (Colours::white); g.fillEllipse (bx - 5, by - 5, 10, 10);
        const float fy = r.getBottom() - r.getHeight() * 0.12f, up = std::sin (phase * 3) > 0.6f ? -0.4f : 0.4f;
        g.setColour (Colour (0xffffb020));
        g.drawLine (cx - r.getWidth() * 0.18f, fy, cx - r.getWidth() * 0.18f + std::cos (up) * r.getWidth() * 0.12f, fy + std::sin (up) * r.getWidth() * 0.12f, 5.0f);
        g.drawLine (cx + r.getWidth() * 0.18f, fy, cx + r.getWidth() * 0.18f - std::cos (0.4f) * r.getWidth() * 0.12f, fy + std::sin (0.4f) * r.getWidth() * 0.12f, 5.0f);
    }

    // ---------------------------------------------------------------- SOUND INVADERS ----------------------------------------------------------------
    Rectangle<float> fieldRect() const { const auto s = stage().toFloat(); const float h = s.getHeight(), w = std::min (s.getWidth() - 2 * 290.0f, h * 1.3f); return { s.getCentreX() - w * 0.5f, s.getY(), w, h }; }
    Point<float> toField (Point<float> p) const { const auto f = fieldRect(); return { (p.x - f.getX()) / f.getWidth(), (p.y - f.getY()) / f.getHeight() }; }
    void drawInvaders (Graphics& g, Rectangle<float> st)
    {
        static const uint8 sp[3][2][6] { { { 0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24 }, { 0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x5a } },
                                         { { 0x42, 0x3c, 0x5a, 0xff, 0xbd, 0x24 }, { 0x42, 0x3c, 0x5a, 0xff, 0xbd, 0x42 } },
                                         { { 0x3c, 0x7e, 0xdb, 0xff, 0x66, 0xc3 }, { 0x3c, 0x7e, 0xdb, 0xff, 0x66, 0x24 } } };
        static const uint8 ship[5] { 0x18, 0x18, 0x7e, 0xff, 0xff };
        auto f = fieldRect();
        const float sh = shake * 6.0f;
        f = f.translated (std::sin (phase * 60) * sh, std::cos (phase * 47) * sh);
        arenaBack (g, f, gameColour (0));
        auto map = [f] (float x, float y) { return Point<float> (f.getX() + x * f.getWidth(), f.getY() + y * f.getHeight()); };
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (f.toNearestInt());
            for (int i = 0; i < 70; ++i) { Random q (i * 7 + 3); const float y = std::fmod (q.nextFloat() + phase * (0.01f + 0.03f * q.nextFloat()), 1.0f); g.setColour (Colours::white.withAlpha (0.15f + 0.3f * q.nextFloat())); const auto p = map (q.nextFloat(), y); g.fillRect (p.x, p.y, 1.5f, 1.5f); }
            const float px = f.getWidth() * kk::game::Invaders::enemyW / 8.0f;
            const int fr = (int) (phase * 2.5f) % 2;
            for (auto& e : inv.enemies)
                if (e.alive) { const auto p = map (e.x, e.y); pixels (g, sp[e.row % 3][fr], 6, p.x - px * 4, p.y - px * 3, px, rowColour (e.row)); }
            if (inv.boss && inv.bossHp > 0) drawBoss (g, map (inv.bossX, inv.bossY), f.getWidth() * kk::game::Invaders::bossW, f.getHeight() * kk::game::Invaders::bossH);
            for (auto& s : inv.shots)
            {
                const auto p = map (s.x, s.y);
                if (s.enemy) { g.setColour (Colour (0xffff5a1f)); for (int k = 0; k < 3; ++k) g.fillRect (p.x - 2.0f + (k % 2 == 0 ? 0.0f : 2.0f), p.y - 6.0f + (float) k * 4.0f, 3.0f, 4.0f); }
                else { g.setColour (Colour (0xff36ff6a).withAlpha (0.35f)); g.fillRect (p.x - 3.0f, p.y - 9.0f, 6.0f, 18.0f); g.setColour (Colours::white); g.fillRect (p.x - 1.0f, p.y - 8.0f, 2.0f, 16.0f); }
            }
            if (inv.invuln <= 0 || std::fmod (phase, 0.2f) < 0.1f) { const auto p = map (inv.px, kk::game::Invaders::playerY); const float spx = f.getWidth() * 0.06f / 8.0f; pixels (g, ship, 5, p.x - spx * 4, p.y - spx * 2.5f, spx, Colour (0xff36ff6a)); }
            g.setColour (Colour (0xff36ff6a).withAlpha (0.4f)); g.drawHorizontalLine ((int) map (0, 0.95f).y, f.getX() + 10, f.getRight() - 10);
            drawSparks (g, kk::game::gInvaders, map);
        }
        // the HUD
        const auto lpan = Rectangle<float> (st.getX(), st.getY(), fieldRect().getX() - st.getX() - 24, st.getHeight()), rpan = Rectangle<float> (fieldRect().getRight() + 24, st.getY(), st.getRight() - fieldRect().getRight() - 24, st.getHeight());
        sidePanel (g, lpan, gameColour (0)); sidePanel (g, rpan, gameColour (0));
        const auto lp = lpan.reduced (18, 0).withTrimmedTop (20).withTrimmedBottom (80);
        hudBlock (g, lp.withHeight (64), "SCORE", String (inv.score), Colours::white);
        hudBlock (g, lp.withHeight (64).translated (0, 76), "WAVE", String (inv.wave) + (inv.bossWave() ? "  BOSS" : ""), rowColour (inv.wave % 5));
        hudBlock (g, lp.withHeight (64).translated (0, 152), "LEVEL", String (inv.level()), Colour (0xffffc63a));
        auto lv = lp.withHeight (64).translated (0, 228);
        hudBlock (g, lv, "LIVES", {}, Colour (0xff36ff6a));
        static const uint8 ship2[5] { 0x18, 0x18, 0x7e, 0xff, 0xff };
        for (int i = 0; i < inv.lives; ++i) pixels (g, ship2, 5, lv.getX() + 14 + (float) i * 40, lv.getY() + 30, 3.5f, Colour (0xff36ff6a));
        if (inv.combo > 1) label (g, "COMBO x" + String (inv.combo), lp.withHeight (40).translated (0, 310), 20.0f, Colour (0xffffd23f), Justification::centredLeft, 0.15f);
        const auto rp = rpan.reduced (18, 20);
        label (g, "THE KEY", rp.withHeight (18), 11.5f, Colours::white.withAlpha (0.55f), Justification::centredLeft, 0.3f);
        label (g, String (kk::mel::keyName (key().root)) + " " + kk::mel::scaleName (key().scale()), rp.withHeight (30).translated (0, 18), 17.0f, Colours::white, Justification::centredLeft, 0.08f);
        g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (12.0f, true, 0.02f)); g.drawFittedText ("every kill is a note of it - the top rows sing higher, a fast combo climbs", rp.withHeight (40).translated (0, 52).toNearestInt(), Justification::topLeft, 3, 0.9f);
        if (inv.boss)
        {
            const auto br = rp.withHeight (60).translated (0, 120);
            label (g, "808 BOSS", br.withHeight (22), 16.0f, Colour (0xffff3b5c), Justification::centredLeft, 0.25f);
            g.setColour (Colour (0xffff3b5c).withAlpha (0.2f)); g.fillRoundedRectangle (br.withY (br.getY() + 28).withHeight (12), 6);
            g.setColour (Colour (0xffff3b5c)); g.fillRoundedRectangle (br.withY (br.getY() + 28).withHeight (12).withWidth (br.getWidth() * std::max (0.0f, inv.bossHp / inv.bossMax)), 6);
        }
        label (g, "MOVE  left / right, A / D, mouse\nSHOOT  space, up, click\nPAUSE  P      LEAVE  ESC", rp.withTop (rp.getBottom() - 70), 12.0f, Colours::white.withAlpha (0.5f), Justification::bottomLeft, 0.05f);
    }
    void drawBoss (Graphics& g, Point<float> c, float w, float h)
    {
        // the 808 BOSS: a giant speaker with angry eyes; it pumps with every hit
        const float pump = 1.0f + 0.06f * inv.bossFlash + 0.02f * std::sin (phase * 8);
        auto r = Rectangle<float> (w * pump, h * pump).withCentre (c);
        g.setColour (Colour (0xff1a0a12)); g.fillRoundedRectangle (r, 8);
        g.setColour (Colour (0xffff3b5c).interpolatedWith (Colours::white, inv.bossFlash * 0.6f)); g.drawRoundedRectangle (r, 8, 2.5f);
        for (int k = 0; k < 2; ++k)
        {
            const float cx = r.getX() + r.getWidth() * (k == 0 ? 0.28f : 0.72f), cy = r.getCentreY(), rr = r.getHeight() * 0.36f;
            for (int ring = 3; ring >= 1; --ring) { g.setColour (Colour (0xffff3b5c).withAlpha (0.18f * (float) ring)); g.drawEllipse (cx - rr * (float) ring / 3, cy - rr * (float) ring / 3, rr * 2 * (float) ring / 3, rr * 2 * (float) ring / 3, 2.0f); }
            g.setColour (Colours::white); g.fillEllipse (cx - 3, cy - 3, 6, 6);
        }
        label (g, "808", r.withSizeKeepingCentre (r.getWidth() * 0.3f, r.getHeight() * 0.5f), 18.0f, Colour (0xffffc63a), Justification::centred, 0.2f);
    }

    // ---------------------------------------------------------------- PIXEL DUEL ----------------------------------------------------------------
    // a pixel fighter on a grid of u: feet at (x, gy)
    static void fighter (Graphics& g, float x, float gy, float u, int facing, int act, Colour c, float ph, bool hurtFlash)
    {
        using D = kk::game::Duel;
        auto px = [&] (float gx, float gyy, float w, float h, Colour col) { g.setColour (col); g.fillRect (std::round (x + (facing > 0 ? gx : -gx - w) * u), std::round (gy - (gyy + h) * u), std::round (w * u), std::round (h * u)); };
        const Colour body = hurtFlash ? Colours::white : c, dark = c.darker (0.7f), skin = Colour (0xfff2c9a0);
        if (act == D::aKo) { px (-8, 0, 16, 4, body); px (8, 0, 4, 4, skin); px (-11, 0, 3, 2, dark); return; }
        const bool walk = act == D::aWalk; const float step = walk ? std::sin (ph * 14.0f) : 0.0f;
        const float lean = act == D::aHurt ? -1.5f : act == D::aPunch ? 1.0f : 0.0f;
        const float lift = act == D::aJump ? 2.0f : 0.0f;
        px (-3 + step, lift, 2, 7 - lift, dark); px (1 - step, lift, 2, 7 - lift, dark);             // legs
        px (-3 + step - 0.5f, lift, 3, 1, Colours::black.withAlpha (0.6f)); px (1 - step, lift, 3, 1, Colours::black.withAlpha (0.6f));
        px (-4 + lean, 7, 8, 8, body);                                                                  // torso
        px (-4 + lean, 9, 8, 1, dark);                                                                  // the belt
        px (-3 + lean * 1.2f, 15, 6, 6, skin);                                                         // head
        px (-3 + lean * 1.2f, 19, 6, 2, dark);                                                          // hair / band
        px (1 + lean * 1.2f, 17, 1.5f, 1.5f, Colours::black);                                          // the eye looks forward
        if (act == D::aPunch) { px (3 + lean, 12, 7, 2, body); px (10 + lean, 11.5f, 3, 3, skin); }     // the punch
        else if (act == D::aSpecial) { px (3 + lean, 12, 5, 2, body); px (3 + lean, 9.5f, 5, 2, body); px (8 + lean, 9, 3, 6, c.brighter (0.6f)); }
        else if (act == D::aHurt) { px (-7 + lean, 13, 3, 2, body); }
        else { px (3 + lean, 8 + std::sin (ph * 4.0f) * 0.5f, 2, 6, body); px (3 + lean, 7 + std::sin (ph * 4.0f) * 0.5f, 2, 2, skin); px (-6 + lean, 9, 2, 5, dark); }
    }
    void drawDuel (Graphics& g, Rectangle<float> st)
    {
        auto ar = st.translated (std::sin (phase * 60) * shake * 7, std::cos (phase * 51) * shake * 5);
        const auto rc = Colour (kk::game::rivalColour (duel.level));
        arenaBack (g, ar, gameColour (1));
        const float u = ar.getHeight() / 120.0f, gy = ar.getBottom() - u * 16;
        {
            Graphics::ScopedSaveState ss (g); Path clip; clip.addRoundedRectangle (ar, 18); g.reduceClipRegion (clip);
            // a pixel skyline + a neon floor
            g.setGradientFill (ColourGradient (Colour (0xff2b0f3c), ar.getX(), ar.getY(), Colour (0xff070512), ar.getX(), gy, false)); g.fillRect (ar.withBottom (gy));
            g.setColour (Colour (0xffffb020).withAlpha (0.8f)); g.fillEllipse (ar.getCentreX() - u * 22, gy - u * 60, u * 44, u * 44);
            for (int k = 0; k < 6; ++k) { g.setColour (Colour (0xff2b0f3c)); g.fillRect (ar.getCentreX() - u * 24, gy - u * 40 + (float) k * u * 4.5f, u * 48, u * (1.0f + (float) k * 0.5f)); }
            for (int i = 0; i < 26; ++i)
            {
                Random q (i * 5 + 1); const float w = u * (8 + q.nextInt (14)), hh = u * (10 + q.nextInt (44)), x = ar.getX() + (float) i * ar.getWidth() / 24.0f - u * 6;
                g.setColour (Colour (0xff120a22)); g.fillRect (std::round (x), std::round (gy - hh), std::round (w), std::round (hh));
                for (int wy = 0; wy < (int) (hh / (u * 5)); ++wy) for (int wx = 0; wx < (int) (w / (u * 4)); ++wx) if (q.nextFloat() < 0.25f) { g.setColour (Colour (0xffffd23f).withAlpha (0.35f)); g.fillRect (std::round (x + u + (float) wx * u * 4), std::round (gy - hh + u * 2 + (float) wy * u * 5), std::round (u * 1.5f), std::round (u * 2)); }
            }
            g.setColour (Colour (0xff1a0a2a)); g.fillRect (ar.withTop (gy));
            for (int i = 0; i < 18; ++i) { const float yy = gy + u * (float) (i * i) * 0.12f; g.setColour (Colour (0xffff4fd8).withAlpha (0.35f - (float) i * 0.015f)); g.fillRect (ar.getX(), yy, ar.getWidth(), 1.5f); }
            for (int i = -12; i <= 12; ++i) { g.setColour (Colour (0xffff4fd8).withAlpha (0.2f)); g.drawLine (ar.getCentreX() + (float) i * u * 8, gy, ar.getCentreX() + (float) i * u * 30, ar.getBottom(), 1.0f); }
            auto fx = [&] (float x) { return ar.getX() + ar.getWidth() * (0.04f + 0.92f * x); };
            const float yScale = ar.getHeight() * 1.1f;
            for (auto& w : duel.waves)
            {
                const float x = fx (w.x), y = gy - (w.y + 0.04f) * yScale; const auto col = w.owner == 0 ? Colour (0xff36d6ff) : rc;
                for (int k = 0; k < 4; ++k) { const float rr = u * (5.0f + (float) k * 4.0f) + std::fmod (phase * 30, u * 4); g.setColour (col.withAlpha (0.8f - (float) k * 0.18f)); g.drawEllipse (x - rr * 0.5f + (w.vx > 0 ? -rr * 0.4f : rr * 0.4f), y - rr, rr, rr * 2, 2.5f); }
            }
            for (int i = 0; i < 2; ++i)
            {
                const auto& F = duel.f[i];
                g.setColour (Colours::black.withAlpha (0.4f)); g.fillEllipse (fx (F.x) - u * 7, gy - u, u * 14, u * 2.5f);
                fighter (g, fx (F.x), gy - F.y * yScale, u * 1.25f, F.facing, F.act, i == 0 ? Colour (0xff36d6ff) : rc, phase + (float) i, F.hurt > 0.15f);
            }
            drawSparks (g, kk::game::gDuel, [&] (float x, float y) { return Point<float> (fx (x), gy - y * yScale - u * 10); });
        }
        // HP bars, names, the sounds, the special meters, the clock
        auto bar = [&] (int who, Rectangle<float> r)
        {
            const auto& F = duel.f[who]; const float frac = jlimit (0.0f, 1.0f, F.hp / duel.maxHp (who)); const auto col = who == 0 ? Colour (0xff36d6ff) : rc;
            g.setColour (Colours::black.withAlpha (0.55f)); g.fillRect (r);
            const int segs = 20; const float sw = r.getWidth() / (float) segs;
            for (int k = 0; k < segs; ++k)
            {
                const bool on = (float) k < frac * (float) segs; const int kk2 = who == 0 ? k : segs - 1 - k;
                g.setColour (on ? (frac < 0.3f ? Colour (0xffff3b5c) : col) : col.withAlpha (0.12f));
                g.fillRect (r.getX() + (float) kk2 * sw + 1, r.getY() + 2, sw - 2, r.getHeight() - 4);
            }
            const auto cm = r.translated (0, r.getHeight() + 6).withHeight (6).withWidth (r.getWidth() * 0.45f);
            const auto cmr = who == 0 ? cm : cm.withX (r.getRight() - cm.getWidth());
            g.setColour (col.withAlpha (0.18f)); g.fillRect (cmr);
            g.setColour (F.charge >= 1.0f ? Colour (0xffffd23f) : col); g.fillRect (who == 0 ? cmr.withWidth (cmr.getWidth() * F.charge) : cmr.withLeft (cmr.getRight() - cmr.getWidth() * F.charge));
            label (g, F.charge >= 1.0f ? "SPECIAL READY" : "SPECIAL", cmr.translated (0, 8).withHeight (16), 11.0f, F.charge >= 1.0f ? Colour (0xffffd23f) : Colours::white.withAlpha (0.5f), who == 0 ? Justification::centredLeft : Justification::centredRight, 0.2f);
        };
        const float bw = st.getWidth() * 0.36f;
        bar (0, Rectangle<float> (st.getX() + 24, st.getY() + 50, bw, 22));
        bar (1, Rectangle<float> (st.getRight() - 24 - bw, st.getY() + 50, bw, 22));
        label (g, "YOU", Rectangle<float> (st.getX() + 24, st.getY() + 14, bw, 30), 22.0f, Colour (0xff36d6ff), Justification::centredLeft, 0.2f);
        label (g, "plays: " + mySound, Rectangle<float> (st.getX() + 24, st.getY() + 104, bw, 18), 12.5f, Colours::white.withAlpha (0.65f), Justification::centredLeft, 0.05f);
        label (g, kk::game::rivalName (duel.level), Rectangle<float> (st.getRight() - 24 - bw, st.getY() + 14, bw, 30), 22.0f, rc, Justification::centredRight, 0.2f);
        g.setColour (Colour (kk::game::rarityColour (rival.rarity)));
        label (g, "carries: " + rival.name + "  (" + kk::game::rarityName (rival.rarity) + ")  -  win to steal it", Rectangle<float> (st.getRight() - 24 - bw, st.getY() + 104, bw, 18), 12.5f, Colour (kk::game::rarityColour (rival.rarity)), Justification::centredRight, 0.05f);
        label (g, String ((int) std::ceil (duel.timeLeft)), Rectangle<float> (st.getCentreX() - 50, st.getY() + 34, 100, 50), 40.0f, Colours::white, Justification::centred, 0.0f);
        label (g, "ROUND " + String (duel.level), Rectangle<float> (st.getCentreX() - 80, st.getY() + 12, 160, 20), 12.0f, Colours::white.withAlpha (0.6f), Justification::centred, 0.3f);
        label (g, "move  left / right     jump  up     punch  Z / click     special  X / right-click     pause  P     leave  ESC", Rectangle<float> (st.getX() + 180, st.getBottom() - 40, st.getWidth() - 200, 24), 12.5f, Colours::white.withAlpha (0.55f), Justification::centred, 0.05f);
    }

    // ---------------------------------------------------------------- PINBALL ----------------------------------------------------------------
    Rectangle<float> pinTable() const { const auto s = stage().toFloat(); const float h = s.getHeight(), w = h / kk::game::Pinball::H; return { s.getCentreX() - w * 0.5f, s.getY(), w, h }; }
    Rectangle<float> pinLane() const { const auto t = pinTable(); return { t.getX() + t.getWidth() * kk::game::Pinball::laneX, t.getY() + t.getHeight() * 0.6f, t.getWidth() * (1.0f - kk::game::Pinball::laneX) + 10, t.getHeight() * 0.4f }; }
    void drawPinball (Graphics& g, Rectangle<float> st)
    {
        using P = kk::game::Pinball;
        auto t = pinTable().translated (std::sin (phase * 60) * shake * 5, 0);
        auto M = [t] (float x, float y) { return Point<float> (t.getX() + x * t.getWidth(), t.getY() + y / P::H * t.getHeight()); };
        const float k = t.getWidth();
        g.setGradientFill (ColourGradient (Colour (0xff1a0b2e), t.getX(), t.getY(), Colour (0xff060814), t.getX(), t.getBottom(), false)); g.fillRoundedRectangle (t, 18);
        {
            Graphics::ScopedSaveState ss (g); Path clip; clip.addRoundedRectangle (t, 18); g.reduceClipRegion (clip);
            for (int i = 0; i < 9; ++i) { g.setColour (Colour (0xffffb020).withAlpha (0.04f)); g.drawEllipse (M (0.5f, 0.7f).x - k * 0.1f * (float) i, M (0.5f, 0.7f).y - k * 0.1f * (float) i, k * 0.2f * (float) i, k * 0.2f * (float) i, 1.0f); }
            // walls: neon
            for (auto& s : pin.segs)
            {
                const auto a = M (s.x0, s.y0), b = M (s.x1, s.y1);
                const auto col = s.kind == 1 ? Colour (kk::game::ingredient (s.ing).colour) : Colour (0xffffb020);
                g.setColour (col.withAlpha (0.18f)); g.drawLine (a.x, a.y, b.x, b.y, 9.0f);
                g.setColour (col); g.drawLine (a.x, a.y, b.x, b.y, s.kind == 1 ? 4.0f : 2.5f);
            }
            for (auto& r : pin.ramps)
            {
                const auto a = M (r.x0, r.y0), b = M (r.x1, r.y1); const auto col = Colour (kk::game::ingredient (r.ing).colour);
                const auto rr = Rectangle<float> (a, b);
                g.setColour (col.withAlpha (0.1f + 0.4f * r.flash)); g.fillRoundedRectangle (rr, 6);
                g.setColour (col.withAlpha (0.8f)); g.drawRoundedRectangle (rr, 6, 1.5f);
                for (int c = 0; c < 3; ++c) { const float yy = rr.getBottom() - 6 - (float) c * 9 - std::fmod (phase * 20, 9.0f); Path ar; ar.addTriangle (rr.getCentreX() - 6, yy, rr.getCentreX() + 6, yy, rr.getCentreX(), yy - 6); g.setColour (col.withAlpha (0.6f)); g.fillPath (ar); }
                label (g, String (kk::game::ingredient (r.ing).name) + " RAMP", rr.translated (0, rr.getHeight() + 2).withHeight (14).expanded (30, 0), 10.0f, col, Justification::centred, 0.15f);
            }
            for (auto& b : pin.bumpers)
            {
                const auto c = M (b.x, b.y); const float r = b.r * k; const auto col = Colour (kk::game::ingredient (b.ing).colour);
                g.setColour (col.withAlpha (0.12f + 0.45f * b.flash)); g.fillEllipse (c.x - r * 1.5f, c.y - r * 1.5f, r * 3, r * 3);
                g.setGradientFill (ColourGradient (col.brighter (0.4f).withAlpha (0.9f), c.x - r * 0.4f, c.y - r * 0.5f, col.darker (0.8f), c.x + r, c.y + r, true)); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
                g.setColour (Colours::white.withAlpha (0.5f + 0.5f * b.flash)); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 2.0f);
                label (g, kk::game::ingredient (b.ing).name, Rectangle<float> (c.x - r * 1.4f, c.y - 8, r * 2.8f, 16), 10.5f, Colours::black.withAlpha (0.85f), Justification::centred, 0.1f);
                label (g, kk::mel::keyName (key().pitch (b.degree) % 12), Rectangle<float> (c.x - 10, c.y + 6, 20, 14), 10.0f, Colours::black.withAlpha (0.6f), Justification::centred, 0.0f);
            }
            for (int i = 0; i < 2; ++i)
            {
                const auto a = M (pin.fl[(size_t) i].px, pin.fl[(size_t) i].py); const auto tp = pin.tip (i); const auto b = M (tp.x, tp.y);
                Path f; const float w0 = P::flipR * k * 1.3f, w1 = P::flipR * k * 0.8f;
                const auto d = (b - a); const float len = d.getDistanceFromOrigin(); const Point<float> n (-d.y / len, d.x / len);
                f.startNewSubPath (a + n * w0); f.lineTo (b + n * w1); f.lineTo (b - n * w1); f.lineTo (a - n * w0); f.closeSubPath();
                g.setColour (Colour (0xffffb020)); g.fillPath (f);
                g.setColour (Colours::white.withAlpha (0.8f)); g.strokePath (f, PathStrokeType (1.4f));
                g.fillEllipse (a.x - w0, a.y - w0, w0 * 2, w0 * 2);
            }
            // the plunger: a spring that squeezes as it charges
            const auto pl = M (0.96f, P::plungerY), plB = M (0.96f, P::H);
            const float sq = pin.power * (plB.y - pl.y) * 0.6f;
            g.setColour (Colour (0xffff3b5c)); g.fillRect (pl.x - k * 0.03f, pl.y + sq, k * 0.06f, 5.0f);
            Path spring; spring.startNewSubPath (pl.x, pl.y + sq + 5);
            for (int z = 1; z <= 8; ++z) spring.lineTo (pl.x + (z % 2 == 0 ? -1.0f : 1.0f) * k * 0.022f, pl.y + sq + 5 + (plB.y - pl.y - sq - 5) * (float) z / 8.0f);
            g.setColour (Colours::white.withAlpha (0.6f)); g.strokePath (spring, PathStrokeType (1.5f));
            for (int i = 0; i < (int) trail.size(); ++i) { const auto p = M (trail[(size_t) i].x, trail[(size_t) i].y); const float r = P::R * k * (0.3f + 0.7f * (float) i / (float) trail.size()); g.setColour (Colours::white.withAlpha (0.06f * (float) i)); g.fillEllipse (p.x - r, p.y - r, r * 2, r * 2); }
            const auto bc = M (pin.bx, pin.by); const float br = P::R * k;
            g.setGradientFill (ColourGradient (Colours::white, bc.x - br * 0.4f, bc.y - br * 0.4f, Colour (0xff8a93a6), bc.x + br, bc.y + br, true)); g.fillEllipse (bc.x - br, bc.y - br, br * 2, br * 2);
            drawSparks (g, kk::game::gPinball, M);
            if (pin.inLane) label (g, "HOLD SPACE (or the lane) - LET GO", Rectangle<float> (M (0.0f, 1.5f).x, M (0, 1.5f).y, t.getWidth() * 0.9f, 20), 11.0f, Colours::white.withAlpha (0.5f + 0.4f * std::sin (phase * 5)), Justification::centred, 0.15f);
        }
        g.setColour (Colour (0xffffb020).withAlpha (0.5f)); g.drawRoundedRectangle (t, 18, 1.5f);
        // left: THE POT - what the run is cooking
        const auto lpan = Rectangle<float> (st.getX(), st.getY(), pinTable().getX() - st.getX() - 24, st.getHeight()), rpan = Rectangle<float> (pinTable().getRight() + 24, st.getY(), st.getRight() - pinTable().getRight() - 24, st.getHeight());
        sidePanel (g, lpan, gameColour (2)); sidePanel (g, rpan, gameColour (2));
        const auto lp = lpan.reduced (18, 16);
        label (g, "THE POT", lp.withHeight (24), 16.0f, Colours::white, Justification::centredLeft, 0.3f);
        label (g, "every bumper, ramp and slingshot throws its ingredient in", lp.withHeight (18).translated (0, 26), 12.0f, Colours::white.withAlpha (0.55f), Justification::centredLeft, 0.02f);
        const int mx = std::max (1, *std::max_element (pin.cooked.begin(), pin.cooked.end()));
        for (int i = 0; i < kk::game::numIngredients; ++i)
        {
            const auto row = Rectangle<float> (lp.getX(), lp.getY() + 60 + (float) i * 34, lp.getWidth(), 28);
            const auto col = Colour (kk::game::ingredient (i).colour); const int n = pin.cooked[(size_t) i];
            g.setColour (col.withAlpha (n > 0 ? 1.0f : 0.3f)); g.fillEllipse (row.getX(), row.getCentreY() - 6, 12, 12);
            label (g, kk::game::ingredient (i).name, row.withX (row.getX() + 22).withWidth (90), 13.0f, Colours::white.withAlpha (n > 0 ? 0.9f : 0.4f), Justification::centredLeft, 0.15f);
            const auto barR = Rectangle<float> (row.getX() + 116, row.getCentreY() - 5, row.getWidth() - 160, 10);
            g.setColour (col.withAlpha (0.12f)); g.fillRoundedRectangle (barR, 5);
            if (n > 0) { g.setColour (col); g.fillRoundedRectangle (barR.withWidth (std::max (8.0f, barR.getWidth() * (float) n / (float) mx)), 5); }
            label (g, String (n), row.withLeft (row.getRight() - 36), 13.0f, Colours::white.withAlpha (0.7f), Justification::centredRight, 0.0f);
        }
        const auto dsh = pin.dish();
        const auto dr = Rectangle<float> (lp.getX(), lp.getY() + 60 + 9 * 34 + 16, lp.getWidth(), 90);
        g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (dr, 12);
        label (g, "COOKING", dr.reduced (14, 8).withHeight (16), 11.0f, Colours::white.withAlpha (0.5f), Justification::centredLeft, 0.3f);
        label (g, pin.hitsTotal > 0 ? dsh.name : String ("nothing yet - hit something!"), dr.reduced (14, 8).withTrimmedTop (18).withHeight (28), 19.0f, Colour (0xffffb020), Justification::centredLeft, 0.05f);
        label (g, String (kk::alc::exciters()[(size_t) dsh.exciter].name) + "  x  " + kk::alc::bodies()[(size_t) dsh.body].name + "  -  " + kk::alc::matterWord (dsh.matter).toUpperCase(),
               dr.reduced (14, 8).withTrimmedTop (50).withHeight (18), 11.5f, Colours::white.withAlpha (0.6f), Justification::centredLeft, 0.1f);
        // right: score, balls, the melody the bounces make
        const auto rp = rpan.reduced (18, 16);
        hudBlock (g, rp.withHeight (64), "SCORE", String (pin.score), Colours::white);
        hudBlock (g, rp.withHeight (64).translated (0, 76), "BALLS", String (pin.balls), Colour (0xffffb020));
        auto mr = rp.withHeight (190).translated (0, 160);
        label (g, "THE MELODY  -  " + String (kk::mel::keyName (key().root)) + " " + kk::mel::scaleName (key().scale()), mr.removeFromTop (22), 12.0f, Colours::white.withAlpha (0.6f), Justification::centredLeft, 0.2f);
        g.setColour (Colour (0xff04060c)); g.fillRoundedRectangle (mr, 10);
        if (! pin.notes.empty())
        {
            const float end = std::max (16.0f, pin.notes.back().start + 1.0f), b0 = end - 16.0f;
            int lo = 127, hi = 0; for (auto& n : pin.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
            lo -= 2; hi += 2;
            for (auto& n : pin.notes)
            {
                if (n.start < b0) continue;
                const float x = mr.getX() + 8 + (n.start - b0) / 16.0f * (mr.getWidth() - 16), y = mr.getBottom() - 8 - (float) (n.pitch - lo) / (float) std::max (1, hi - lo) * (mr.getHeight() - 16);
                g.setColour (Colour (0xffffb020).withAlpha (0.35f + 0.65f * n.vel)); g.fillRoundedRectangle (x, y - 3, std::max (5.0f, n.len / 16.0f * (mr.getWidth() - 16)), 6, 3);
            }
        }
        else label (g, "hit the bumpers - each is a note", mr, 12.0f, Colours::white.withAlpha (0.4f), Justification::centred, 0.05f);
        label (g, "PLUNGER  hold space / the lane\nFLIPPERS  Z + M,  left + right mouse\nPAUSE  P      LEAVE  ESC", rp.withTop (mr.getBottom() + 14).withHeight (60), 12.0f, Colours::white.withAlpha (0.5f), Justification::topLeft, 0.05f);
    }

    // ---------------------------------------------------------------- THE CHEST ----------------------------------------------------------------
    Rectangle<float> chestBox() const { const auto s = stage().toFloat(); const float w = opened ? 170.0f : 260.0f, h = opened ? 105.0f : 180.0f; return { s.getCentreX() - w * 0.5f, s.getY() + (opened ? 96.0f : 90.0f), w, h }; }
    Rectangle<float> cardRect (int i) const
    {
        const auto s = stage().toFloat(); const int n = (int) cards.size();
        const float gap = 18, cw = std::min (220.0f, (s.getWidth() - 60 - gap * (float) (n - 1)) / (float) std::max (1, n)), ch = 310;
        const float x0 = s.getCentreX() - ((float) n * cw + (float) (n - 1) * gap) * 0.5f;
        return { x0 + (float) i * (cw + gap), s.getY() + 236, cw, ch };
    }
    void drawChest (Graphics& g, Rectangle<float> st)
    {
        arenaBack (g, st, Colour (kk::game::rarityColour (cards.empty() ? 0 : chest.best())));
        openT = std::min (1.0f, openT + 0.04f);
        label (g, chestTitle, st.withHeight (40).translated (0, 14), 24.0f, Colours::white, Justification::centred, 0.12f);
        if (cards.empty())
        {
            static const int need[] { 300, 200, 2500 };
            label (g, "SCORE " + String (chest.score) + "  -  beat " + String (need[jlimit (0, 2, chest.game)]) + " (or clear the level) to earn a chest", st.withHeight (30).translated (0, 60), 15.0f, Colours::white.withAlpha (0.6f), Justification::centred, 0.05f);
            drawChestBox (g, chestBox(), false, Colour (0xff60687a));
            return;
        }
        const auto best = Colour (kk::game::rarityColour (chest.best()));
        if (opened)
        {
            // rays in the colour of the best sound inside
            const auto c = chestBox().getCentre();
            for (int i = 0; i < 16; ++i)
            {
                const float a = phase * 0.3f + (float) i / 16.0f * MathConstants<float>::twoPi, len = 220.0f * openT;
                Path ray; ray.addTriangle (c.x, c.y, c.x + std::cos (a - 0.07f) * len, c.y + std::sin (a - 0.07f) * len, c.x + std::cos (a + 0.07f) * len, c.y + std::sin (a + 0.07f) * len);
                g.setColour (best.withAlpha (0.12f)); g.fillPath (ray);
            }
        }
        drawChestBox (g, chestBox(), opened, opened ? best : Colour (0xffffc63a));
        if (! opened) { label (g, String ((int) cards.size()) + (cards.size() == 1 ? " SOUND INSIDE" : " SOUNDS INSIDE") + "  -  click the chest", st.withHeight (24).translated (0, 360), 14.0f, Colours::white.withAlpha (0.7f), Justification::centred, 0.2f); return; }
        for (int i = 0; i < (int) cards.size(); ++i) drawCard (g, i);
        if (savedNote.isNotEmpty() || ! saveQueue.empty())
            label (g, saveQueue.empty() ? savedNote : "saving into MY SOUNDS > UNLOCKED ...", st.withHeight (22).translated (0, 236 + 322), 13.0f, Colour (0xff36ff6a), Justification::centred, 0.05f);
        else if (! persist) label (g, "click a sound = hear it on your keys", st.withHeight (22).translated (0, 236 + 322), 13.0f, Colours::white.withAlpha (0.5f), Justification::centred, 0.05f);
    }
    static void drawChestBox (Graphics& g, Rectangle<float> r, bool open, Colour glow)
    {
        const auto wood = Colour (0xff5a3418), band = Colour (0xffd9a441);
        const auto body = r.withTrimmedTop (r.getHeight() * 0.42f);
        for (int k = 4; k >= 1; --k) { g.setColour (glow.withAlpha (0.05f * (float) k)); g.fillRoundedRectangle (r.expanded ((float) k * 7), 16); }
        if (open)
        {
            const auto lid = Rectangle<float> (r.getX(), r.getY() - r.getHeight() * 0.25f, r.getWidth(), r.getHeight() * 0.3f);
            g.setColour (wood.darker (0.3f)); g.fillRoundedRectangle (lid, 10);
            g.setColour (band); g.drawRoundedRectangle (lid, 10, 3.0f);
            g.setColour (glow.withAlpha (0.9f)); g.fillRoundedRectangle (body.withHeight (14).translated (0, -7), 6);
        }
        else
        {
            const auto lid = r.withHeight (r.getHeight() * 0.45f);
            g.setColour (wood.brighter (0.15f)); g.fillRoundedRectangle (lid, 14);
            g.setColour (band); g.drawRoundedRectangle (lid, 14, 3.5f);
            g.fillRect (lid.getCentreX() - 8, lid.getY(), 16.0f, lid.getHeight());
        }
        g.setColour (wood); g.fillRoundedRectangle (body, 10);
        g.setColour (band); g.drawRoundedRectangle (body, 10, 3.5f);
        g.fillRect (body.getX() + body.getWidth() * 0.15f, body.getY(), 10.0f, body.getHeight()); g.fillRect (body.getRight() - body.getWidth() * 0.15f - 10, body.getY(), 10.0f, body.getHeight());
        const auto lock = Rectangle<float> (26, 30).withCentre ({ body.getCentreX(), body.getY() + 6 });
        g.setColour (band.brighter (0.3f)); g.fillRoundedRectangle (lock, 5);
        g.setColour (wood.darker (0.6f)); g.fillEllipse (lock.getCentreX() - 4, lock.getCentreY() - 6, 8, 8); g.fillRect (lock.getCentreX() - 1.5f, lock.getCentreY(), 3.0f, 8.0f);
    }
    void drawCard (Graphics& g, int i)
    {
        const auto& c = cards[(size_t) i];
        const float rise = jlimit (0.0f, 1.0f, openT * 2.0f - (float) i * 0.18f);
        auto r = cardRect (i).translated (0, (1.0f - rise) * 60.0f);
        const auto col = Colour (kk::game::rarityColour (c.r.rarity)); const bool sel = i == selected, hov = i == hover;
        const float a = rise;
        if (c.r.rarity == kk::game::rLegendary) for (int k = 3; k >= 1; --k) { g.setColour (col.withAlpha (0.08f * (float) k * a)); g.drawRoundedRectangle (r.expanded ((float) k * 3), 16, 4.0f); }
        g.setGradientFill (ColourGradient (col.withAlpha (0.32f * a), r.getX(), r.getY(), Colour (0xff070912).withAlpha (a), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 14);
        g.setColour (col.withAlpha ((sel ? 1.0f : hov ? 0.8f : 0.55f) * a)); g.drawRoundedRectangle (r.reduced (0.5f), 14, sel ? 3.0f : 1.5f);
        if (c.r.rarity == kk::game::rLegendary)
        {
            // a golden sheen sweeps over the card
            Graphics::ScopedSaveState ss (g); Path clip; clip.addRoundedRectangle (r, 14); g.reduceClipRegion (clip);
            const float sx = r.getX() - r.getWidth() + std::fmod (phase * 0.6f, 1.6f) * r.getWidth() * 2.2f;
            Path sheen; sheen.addQuadrilateral (sx, r.getY(), sx + 40, r.getY(), sx - 60, r.getBottom(), sx - 100, r.getBottom());
            g.setColour (Colours::white.withAlpha (0.14f * a)); g.fillPath (sheen);
        }
        auto in = r.reduced (14);
        label (g, kk::game::rarityName (c.r.rarity), in.removeFromTop (18), 11.5f, col.withAlpha (a), Justification::centredLeft, 0.35f);
        const String tagTxt = c.r.stolen ? "STOLEN" : c.r.cooked ? "COOKED" : c.isNew ? "NEW!" : "x" + String (c.count);
        label (g, tagTxt, r.reduced (14).withHeight (18), 11.5f, (c.isNew || c.r.stolen || c.r.cooked ? Colour (0xff36ff6a) : Colours::white.withAlpha (0.5f)).withMultipliedAlpha (a), Justification::centredRight, 0.25f);
        // its waveform, drawn from the recipe (glassy = fast and bright, mud = slow and wide)
        const auto wv = in.removeFromTop (110).reduced (0, 10);
        Path p; kk::Rng q; q.seed (c.r.seed);
        const float f1 = 3.0f + 14.0f * (1.0f - c.r.matter), f2 = f1 * (1.5f + q.uni()), dec = 1.5f + 3.0f * (1.0f - c.r.stretch * 0.5f);
        for (int x = 0; x <= 80; ++x)
        {
            const float u = (float) x / 80.0f, env = std::exp (-u * dec) * std::min (1.0f, u * (8.0f - 7.0f * c.r.cool));
            const float y = env * (0.7f * std::sin (u * f1 * MathConstants<float>::twoPi) + 0.3f * std::sin (u * f2 * MathConstants<float>::twoPi + phase * 2.0f * (sel ? 1.0f : 0.0f)));
            const auto pt = Point<float> (wv.getX() + u * wv.getWidth(), wv.getCentreY() - y * wv.getHeight() * 0.45f);
            if (x == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (col.withAlpha (0.25f * a)); g.strokePath (p, PathStrokeType (5.0f));
        g.setColour (col.brighter (0.3f).withAlpha (a)); g.strokePath (p, PathStrokeType (1.8f));
        g.setColour (c.r.rarity == kk::game::rLegendary ? Colour (0xffffd86b).withAlpha (a) : Colours::white.withAlpha (a));
        g.setFont (kk::modern::font (18.0f, true, 0.04f));
        g.drawFittedText (c.r.name, in.removeFromTop (48).toNearestInt(), Justification::centredLeft, 2, 0.8f);
        label (g, String (kk::alc::exciters()[(size_t) jlimit (0, 4, c.r.exciter)].name) + " x " + kk::alc::bodies()[(size_t) jlimit (0, 4, c.r.body)].name, in.removeFromTop (18), 10.5f, Colours::white.withAlpha (0.55f * a), Justification::centredLeft, 0.05f);
        label (g, kk::alc::sizeWord (c.r.size) + kk::alc::matterWord (c.r.matter) + (c.r.split > 0.5f ? "  /  TORN" : "") + (c.r.heat > 0.5f ? "  /  HOT" : "") + (c.r.cool > 0.5f ? "  /  FROZEN" : ""), in.removeFromTop (18), 10.5f, Colours::white.withAlpha (0.55f * a), Justification::centredLeft, 0.05f);
        const auto pl = r.reduced (14).removeFromBottom (30);
        g.setColour (col.withAlpha ((sel ? 0.4f : 0.15f) * a)); g.fillRoundedRectangle (pl, 15);
        label (g, sel ? "ON YOUR KEYS - CLICK AGAIN" : "CLICK = PLAY", pl, 11.5f, Colours::white.withAlpha (a), Justification::centred, 0.2f);
    }

    // ---------------------------------------------------------------- THE COLLECTION BOOK ----------------------------------------------------------------
    Rectangle<float> bookGrid() const { const auto s = stage().toFloat(); const float w = s.getWidth() * 0.66f, cw = (w - 20) / 25.0f, ch = std::min (cw * 1.45f, (s.getHeight() - 80) / 10.0f); return { s.getX() + 14, s.getY() + 60, cw * 25, ch * 10 }; }
    Rectangle<int> bookPanel() const { const auto gr = bookGrid(); const auto s = stage(); return { (int) gr.getRight() + 30, s.getY() + 56, s.getRight() - (int) gr.getRight() - 40, s.getHeight() - 70 }; }
    int slotAt (Point<float> p) const
    {
        const auto gr = bookGrid(); if (! gr.contains (p)) return -1;
        const float cw = gr.getWidth() / 25.0f, ch = gr.getHeight() / 10.0f; const int s = (int) ((p.y - gr.getY()) / ch) * 25 + (int) ((p.x - gr.getX()) / cw);
        return s >= 0 && s < kk::game::numSlots ? s : -1;
    }
    void drawBook (Graphics& g, Rectangle<float> st)
    {
        arenaBack (g, st, Colour (0xffffc63a));
        label (g, "COLLECTION BOOK", Rectangle<float> (st.getX() + 20, st.getY() + 12, 400, 32), 22.0f, Colours::white, Justification::centredLeft, 0.2f);
        label (g, String (coll.unlocked()) + " / " + String (kk::game::numSlots), Rectangle<float> (st.getX() + 300, st.getY() + 12, 200, 32), 22.0f, Colour (0xffffc63a), Justification::centredLeft, 0.05f);
        const auto gr = bookGrid(); const float cw = gr.getWidth() / 25.0f, ch = gr.getHeight() / 10.0f;
        for (int s = 0; s < kk::game::numSlots; ++s)
        {
            const auto r = Rectangle<float> (gr.getX() + (float) (s % 25) * cw, gr.getY() + (float) (s / 25) * ch, cw, ch).reduced (2.5f);
            const int rar = kk::game::rarityOfSlot (s); const auto col = Colour (kk::game::rarityColour (rar)); const int n = coll.count[(size_t) s];
            if (n > 0)
            {
                g.setGradientFill (ColourGradient (col.withAlpha (0.85f), r.getX(), r.getY(), col.darker (0.6f), r.getRight(), r.getBottom(), false)); g.fillRoundedRectangle (r, 5);
                if (rar == kk::game::rLegendary) { g.setColour (Colours::white.withAlpha (0.25f + 0.2f * std::sin (phase * 3 + (float) s))); g.drawRoundedRectangle (r, 5, 1.5f); }
                if (n > 1) label (g, String (n), r.reduced (3, 1), 10.0f, Colours::black.withAlpha (0.75f), Justification::bottomRight, 0.0f);
            }
            else { g.setColour (col.withAlpha (0.07f)); g.fillRoundedRectangle (r, 5); g.setColour (col.withAlpha (0.22f)); g.drawRoundedRectangle (r, 5, 1.0f); label (g, "?", r, 11.0f, col.withAlpha (0.3f), Justification::centred, 0.0f); }
            if (s == bookSel) { g.setColour (Colours::white); g.drawRoundedRectangle (r.expanded (2), 6, 2.0f); }
            else if (s == bookHover) { g.setColour (Colours::white.withAlpha (0.5f)); g.drawRoundedRectangle (r.expanded (1), 6, 1.2f); }
        }
        // the page of one sound
        const auto bp = bookPanel().toFloat();
        const int show = bookHover >= 0 ? bookHover : bookSel;
        if (show >= 0)
        {
            const auto rc = kk::game::recipeForSlot (show); const auto col = Colour (kk::game::rarityColour (rc.rarity)); const bool have = coll.count[(size_t) show] > 0;
            label (g, "No. " + String (show + 1) + "   " + kk::game::rarityName (rc.rarity), bp.withHeight (20), 12.5f, col, Justification::centredLeft, 0.3f);
            g.setColour (have ? (rc.rarity == kk::game::rLegendary ? Colour (0xffffd86b) : Colours::white) : Colours::white.withAlpha (0.4f)); g.setFont (kk::modern::font (26.0f, true, 0.03f));
            g.drawFittedText (have ? rc.name : String ("? ? ?"), bp.withHeight (64).translated (0, 24).toNearestInt(), Justification::centredLeft, 2, 0.8f);
            if (have)
            {
                label (g, String (kk::alc::exciters()[(size_t) rc.exciter].name) + "  x  " + kk::alc::bodies()[(size_t) rc.body].name, bp.withHeight (20).translated (0, 96), 13.0f, Colours::white.withAlpha (0.7f), Justification::centredLeft, 0.08f);
                label (g, kk::alc::sizeWord (rc.size) + kk::alc::matterWord (rc.matter) + "   -   unlocked " + String (coll.count[(size_t) show]) + (coll.count[(size_t) show] == 1 ? " time" : " times"), bp.withHeight (20).translated (0, 120), 13.0f, Colours::white.withAlpha (0.6f), Justification::centredLeft, 0.05f);
            }
            else label (g, rc.rarity == kk::game::rLegendary ? "legendary sounds come from the best runs - beat the 808 BOSS on high levels, or rival 5+" : "not found yet - win levels and beat scores: chests drop it", bp.withHeight (40).translated (0, 96), 12.5f, Colours::white.withAlpha (0.55f), Justification::topLeft, 0.03f);
        }
        else label (g, "point at a sound", bp.withHeight (30), 15.0f, Colours::white.withAlpha (0.5f), Justification::centredLeft, 0.1f);
        {
            const auto how = bp.withTop (bp.getY() + 300).withHeight (120);
            g.setColour (Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (how, 12);
            label (g, "HOW TO FILL THE BOOK", how.reduced (14, 10).withHeight (16), 11.5f, Colour (0xffffc63a), Justification::centredLeft, 0.3f);
            g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (12.0f, true, 0.02f));
            g.drawFittedText ("clear a SOUND INVADERS level (beat the 808 BOSS), win a PIXEL DUEL or score high in PINBALL.  a better score = more sounds in the chest and rarer ones.  the same run always opens the same chest.",
                              how.reduced (14, 10).withTrimmedTop (22).toNearestInt(), Justification::topLeft, 5, 0.9f);
        }
        auto tot = bp.withTop (bp.getBottom() - 4 * 46 - 10);
        for (int r = 0; r < kk::game::numRarities; ++r)
        {
            auto row = tot.removeFromTop (46);
            const auto col = Colour (kk::game::rarityColour (r)); const int all = kk::game::slotStart (r + 1) - kk::game::slotStart (r), have = coll.unlocked (r);
            label (g, String (kk::game::rarityName (r)), row.withHeight (20), 12.5f, col, Justification::centredLeft, 0.3f);
            label (g, String (have) + " / " + String (all), row.withHeight (20), 12.5f, Colours::white.withAlpha (0.7f), Justification::centredRight, 0.05f);
            const auto bar = row.withTrimmedTop (24).withHeight (9);
            g.setColour (col.withAlpha (0.14f)); g.fillRoundedRectangle (bar, 4.5f);
            if (have > 0) { g.setColour (col); g.fillRoundedRectangle (bar.withWidth (std::max (8.0f, bar.getWidth() * (float) have / (float) all)), 4.5f); }
        }
    }

    // ---------------------------------------------------------------- members ----------------------------------------------------------------
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton menuTab { lnf }, bookTab { lnf }, againBtn { lnf }, arcadeBtn { lnf }, openBtn { lnf }, bookPlayBtn { lnf }, backBtn { lnf };
    ArcadeDragButton dragWav { "DRAG WAV", Colour (0xff36ff6a) }, dragMidi { "DRAG MIDI", Colour (0xffffb020) }, bookWav { "DRAG WAV", Colour (0xff36ff6a) };
    int view = vMenu, hover = -1, bookHover = -1, bookSel = -1, selected = -1, lastLevel = 1, invLevel = 1, duelLevel = 1;
    std::array<int, 3> best {};
    kk::game::Invaders inv; kk::game::Duel duel; kk::game::Pinball pin;
    Recipe rival; String mySound;
    kk::game::Chest chest; std::vector<Card> cards; std::vector<int> saveQueue; String chestTitle, savedNote; bool opened = false, chestWin = false;
    kk::live::Result lastMelody;
    std::vector<Spark> sparks; std::vector<kk::game::Pinball::Point2> trail; std::vector<Held> held;
    bool paused = false, persist = true, tapA = false, tapB = false, tapC = false, mouseL = false, mouseR = false, mousePlunger = false, mouseCtl = false;
    float phase = 0, shake = 0, mouseX = 0.5f, acc = 0, openT = 0;
    double lastMs = 0;
    uint32 runSeed = 1;
};
