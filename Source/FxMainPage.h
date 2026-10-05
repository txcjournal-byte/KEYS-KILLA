// v0.42 EVOLVE FX: the mixer plugin - only what belongs on a track or the master.
// MIX LAB (EQ, COMP, TIME MACHINE, SPACE + ECHO, COACH), FX (SURPRISE FX, STEP FX, RACK), CHAINS (ready + your own), LISTEN (sound -> MIDI).
// Included by PluginEditor.cpp (uses its HotButton, MixLabPage, FxRackPage, LabCacheHolder, pageBackdrop).

class ChainsPage : public Component
{
public:
    ChainsPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (saveBtn, "SAVE MY CHAIN", "Save everything you set (MIX LAB + FX) as your own chain", [this] { saveChain(); });
        btn (evolveBtn, "EVOLVE IT", "A new variation of what you have now - again and again until you like it", [this] { proc.chainEvolve (0.45f); note = "evolved - EVOLVE IT again, or SAVE MY CHAIN"; repaint(); if (onChanged) onChanged(); });
        btn (resetBtn, "ALL OFF", "Everything off / flat - the sound passes clean", [this] { proc.chainReset(); sel = -1; note = "all off"; repaint(); if (onChanged) onChanged(); });
        evolveBtn.hero = true;
        refreshList();
    }
    std::function<void()> onChanged;
    void visibilityChanged() override { if (isVisible()) refreshList(); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("FEED", 24, 12, 220, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("one click = a whole chain of effects (MIX LAB + FX).  then change anything, EVOLVE IT, SAVE MY CHAIN", 220, 18, getWidth() - 240, 28, Justification::centredLeft);
        static const uint32 cols[] { 0xff22d3ee, 0xffff8a3d, 0xffb07a4a, 0xffa78bfa, 0xff36ff6a, 0xff4d7dff, 0xffff4fd8, 0xffff3b5c, 0xffffd23f, 0xffff4d6d, 0xff2ee6a6, 0xff7c4dff };
        for (int i = 0; i < KeysKillaProcessor::chainNames().size(); ++i)
        {
            const auto r = tile (i).toFloat();
            const auto c = Colour (cols[i % 12]);
            const bool on = i == sel, hot = i == hover;
            g.setGradientFill (ColourGradient (c.withAlpha (on ? 0.32f : hot ? 0.2f : 0.1f), r.getX(), r.getY(), c.withAlpha (0.03f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 12);
            g.setColour (on ? c : c.withAlpha (hot ? 0.8f : 0.4f)); g.drawRoundedRectangle (r, 12, on ? 2.2f : 1.2f);
            g.setColour (t.text); g.setFont (kk::modern::font (17.0f, true, 0.12f));
            g.drawText (KeysKillaProcessor::chainNames()[i], r.reduced (16, 12).removeFromTop (24), Justification::centredLeft);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, false, 0.0f));
            g.drawFittedText (KeysKillaProcessor::chainHints()[i], r.reduced (16, 12).withTrimmedTop (30).toNearestInt(), Justification::topLeft, 2, 0.85f);
        }
        // your chains
        const auto L = listArea();
        kk::modern::plate (g, L.toFloat(), 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (13.0f, true, 0.25f));
        g.drawText ("MY CHAINS", L.getX() + 16, L.getY() + 10, 200, 18, Justification::centredLeft);
        for (int i = 0; i < (int) mine.size(); ++i)
        {
            const auto r = row (i);
            if (r.getBottom() > L.getBottom() - 6) break;
            g.setColour (i == hoverMine ? t.accent.withAlpha (0.15f) : t.glass); g.fillRoundedRectangle (r.toFloat(), 7);
            g.setColour (t.text); g.setFont (kk::modern::font (13.5f, true, 0.04f));
            g.drawText (mine[(size_t) i].getFileNameWithoutExtension(), r.reduced (12, 0), Justification::centredLeft);
        }
        if (mine.empty()) { g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.03f)); g.drawFittedText ("none yet - set a sound you like and press SAVE MY CHAIN", L.reduced (16, 40), Justification::topLeft, 3); }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.03f)); g.drawText (note, 24, getHeight() - 34, getWidth() - 48, 22, Justification::centredLeft); }
    }
    void resized() override
    {
        const auto L = listArea();
        saveBtn.setBounds (L.getX() + 14, L.getBottom() - 54, L.getWidth() - 28, 40);
        evolveBtn.setBounds (24, getHeight() - 92, 200, 44); resetBtn.setBounds (232, getHeight() - 92, 140, 44);
    }
    void mouseMove (const MouseEvent& e) override
    {
        int h = -1, hm = -1;
        for (int i = 0; i < KeysKillaProcessor::chainNames().size(); ++i) if (tile (i).contains (e.getPosition())) h = i;
        for (int i = 0; i < (int) mine.size(); ++i) if (row (i).contains (e.getPosition())) hm = i;
        if (h != hover || hm != hoverMine) { hover = h; hoverMine = hm; repaint(); }
    }
    void mouseExit (const MouseEvent&) override { hover = hoverMine = -1; repaint(); }
    void mouseUp (const MouseEvent& e) override
    {
        for (int i = 0; i < KeysKillaProcessor::chainNames().size(); ++i)
            if (tile (i).contains (e.getPosition())) { proc.chainApply (i); sel = i; note = KeysKillaProcessor::chainNames()[i] + " - change anything in MIX LAB / FX"; repaint(); if (onChanged) onChanged(); return; }
        for (int i = 0; i < (int) mine.size(); ++i)
            if (row (i).contains (e.getPosition()))
            {
                if (e.mods.isPopupMenu())
                {
                    PopupMenu m; m.addItem (1, "Delete " + mine[(size_t) i].getFileNameWithoutExtension());
                    m.showMenuAsync (PopupMenu::Options(), [this, f = mine[(size_t) i], safe = SafePointer<ChainsPage> (this)] (int r) { if (safe != nullptr && r == 1) { f.deleteFile(); refreshList(); repaint(); } });
                    return;
                }
                note = proc.chainLoad (mine[(size_t) i]) ? mine[(size_t) i].getFileNameWithoutExtension() + " loaded" : String ("could not read it");
                sel = -1; repaint(); if (onChanged) onChanged(); return;
            }
    }
private:
    Rectangle<int> gridArea() const { return { 24, 66, getWidth() - 24 - 360, getHeight() - 66 - 110 }; }
    Rectangle<int> tile (int i) const
    {
        const auto G = gridArea();
        const int cw = (G.getWidth() - 3 * 14) / 4, ch = (G.getHeight() - 2 * 14) / 3;
        return { G.getX() + (i % 4) * (cw + 14), G.getY() + (i / 4) * (ch + 14), cw, ch };
    }
    Rectangle<int> listArea() const { return { getWidth() - 344, 66, 320, getHeight() - 66 - 40 }; }
    Rectangle<int> row (int i) const { const auto L = listArea(); return { L.getX() + 12, L.getY() + 36 + i * 38, L.getWidth() - 24, 32 }; }
    void refreshList()
    {
        mine.clear();
        for (auto& f : KeysKillaProcessor::chainFolder().findChildFiles (File::findFiles, false, "*.evochain")) mine.push_back (f);
        std::sort (mine.begin(), mine.end(), [] (const File& a, const File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
        repaint();
    }
    void saveChain()
    {
        auto* w = new AlertWindow ("SAVE MY CHAIN", "A name for this chain", MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", sel >= 0 ? KeysKillaProcessor::chainNames()[sel] + " mine" : String ("My chain"));
        w->addButton ("SAVE", 1, KeyPress (KeyPress::returnKey)); w->addButton ("CANCEL", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([this, w, safe = SafePointer<ChainsPage> (this)] (int r)
        {
            if (safe == nullptr || r != 1) return;
            const auto f = proc.chainSave (w->getTextEditorContents ("name").trim());
            note = f.existsAsFile() ? "saved: " + f.getFileNameWithoutExtension() + "  (Documents / KEYS KILLA / FX Chains)" : String ("could not save");
            refreshList();
        }), true);
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton saveBtn { lnf }, evolveBtn { lnf }, resetBtn { lnf };
    std::vector<File> mine;
    String note;
    int sel = -1, hover = -1, hoverMine = -1;
};

// LISTEN: the sound of the track becomes MIDI notes
class ListenPage : public Component, private Timer
{
public:
    ListenPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (listenBtn, "LISTEN", "Press it, press PLAY in FL, press it again when the melody has played once", [this]
        {
            if (proc.melListening()) { proc.melListen (false); note = proc.melHasMine ? "got it: " + String ((int) proc.melMine.notes.size()) + " notes, " + kk::mel::keyName (proc.melMine.key) + " " + kk::mel::scaleName (proc.melMine.scale) + "  -  drag the MIDI into FL"
                                                                                    : String ("no clear melody heard - one voice / one instrument works best"); }
            else { proc.melListen (true); note = "listening ... press PLAY in FL"; }
            refresh();
        });
        listenBtn.hero = true;
        dragMidi.makeFile = [this] { return proc.melExport (-2); };
        dragMidi.setTooltip ("The melody as MIDI - drop it on any instrument in FL");
        addChildComponent (dragMidi);
        startTimerHz (20);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("NEURAL EAR", 24, 12, 260, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("the melody on this track (Nexus, a vocal, any plugin) becomes MIDI notes", 300, 18, getWidth() - 240, 28, Justification::centredLeft);
        const auto r = rollArea().toFloat();
        kk::modern::well (g, r, 12.0f);
        if (proc.melHasMine && ! proc.melMine.notes.empty())
        {
            const auto& m = proc.melMine;
            int lo = 127, hi = 0; for (auto& n : m.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
            lo -= 2; hi += 2;
            const float beats = m.beats(), rows = (float) std::max (8, hi - lo + 1);
            auto a = r.reduced (16, 14);
            for (int b = 0; b <= m.bars; ++b) { g.setColour (t.text.withAlpha (b % 4 == 0 ? 0.2f : 0.07f)); g.fillRect (a.getX() + a.getWidth() * (float) b * 4.0f / beats, a.getY(), 1.0f, a.getHeight()); }
            for (auto& n : m.notes)
            {
                const float x = a.getX() + a.getWidth() * n.start / beats, w = std::max (3.0f, a.getWidth() * n.len / beats - 1.0f);
                const float y = a.getBottom() - a.getHeight() * (float) (n.pitch - lo + 1) / rows, h = std::max (4.0f, a.getHeight() / rows - 1.5f);
                g.setColour (t.accent.withAlpha (0.55f + 0.4f * n.vel)); g.fillRoundedRectangle (x, y, w, h, 2.0f);
            }
            g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.1f));
            g.drawText (String (kk::mel::keyName (m.key)) + " " + kk::mel::scaleName (m.scale) + "   " + String (m.bars) + " BARS   " + String ((int) m.notes.size()) + " NOTES", rollArea().translated (0, rollArea().getHeight() + 6).withHeight (20), Justification::centredLeft);
        }
        else
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (16.0f, true, 0.05f));
            g.drawFittedText ("1.  EVOLVE FX PRO sits on the track of the plugin that plays the melody\n2.  press LISTEN, then PLAY in FL\n3.  when the melody has played once, press LISTEN again\n4.  drag the MIDI into FL", rollArea().reduced (30), Justification::centred, 6);
        }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (14.0f, true, 0.03f)); g.drawText (note, 24, getHeight() - 40, getWidth() - 48, 24, Justification::centredLeft); }
        if (proc.melListening())
        {
            const float lvl = jlimit (0.0f, 1.0f, proc.melHeardAudio() * 2.0f);
            g.setColour (Colour (0xff36ff6a).withAlpha (0.25f + 0.6f * lvl)); g.fillEllipse ((float) listenBtn.getRight() + 16, (float) listenBtn.getY() + 12, 20, 20);
        }
    }
    void resized() override
    {
        listenBtn.setBounds (24, 70, 220, 54);
        dragMidi.setBounds (260, 70, 220, 54);
    }
private:
    Rectangle<int> rollArea() const { return { 24, 140, getWidth() - 48, getHeight() - 220 }; }
    void refresh() { listenBtn.setButtonText (proc.melListening() ? "STOP" : "LISTEN"); listenBtn.selected = proc.melListening(); dragMidi.setVisible (proc.melHasMine && ! proc.melListening()); repaint(); }
    void timerCallback() override { if (isShowing() && proc.melListening()) repaint(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton listenBtn { lnf };
    DragFileButton dragMidi { "DRAG MIDI", TC (0xff36ff6a) };
    String note;
};

#include "FxProPages.h"
#include "FxTouchPages.h"
#include "FxWorldPages.h"   // v0.45 WORLDS: CLUB, SEASONING, ENGINE, MOOD WORDS + DRAW

// v0.45 FEWER DOORS: six doors on the left (MIX, TOUCH, ORGANIC, WORLDS, ERAS, EAR), the pages of a door in a strip on top.
// The flat page index stays (showView (i), snapshots 90 + i): 0 MIX LAB, 1 FX RACK, 2 HOLOROOM, 3 GRAB, 4 REMIX REEL, 5 DIAL-UP,
// 6 WARP DRIVE, 7 DRAW (was DOODLE - DOODLE moves to EVOLVE), 8 FINAL BOSS, 9 LIQUID, 10 INTENT, 11 EROSION, 12 FEED, 13 NEURAL EAR,
// 14 CLUB, 15 SEASONING, 16 ENGINE, 17 MOOD WORDS.
class FxMainPage : public Component, private Timer
{
public:
    static constexpr int numPages = 18, numDoors = 6;
    static const std::vector<int>& doorPages (int d)
    {
        static const std::vector<int> p[numDoors] { { 0, 1, 12 }, { 2, 3, 7 }, { 9, 10, 11 }, { 14, 15, 16, 17 }, { 4, 5, 6, 8 }, { 13 } };
        return p[jlimit (0, numDoors - 1, d)];
    }
    static int doorOf (int page) { for (int d = 0; d < numDoors; ++d) for (int p : doorPages (d)) if (p == page) return d; return 0; }
    explicit FxMainPage (KeysKillaProcessor& p) : proc (p)
    {
        lnf.setSkin (Skin::all()[(size_t) kk::themeIndex()]);
        if (auto* c = activeLabCache(); c != nullptr && c->builtFor != kk::themeIndex()) c->rebuild();
        setLookAndFeel (&lnf);
        pages[0] = std::make_unique<MixLabPage> (proc, lnf);
        pages[1] = std::make_unique<FxRackPage> (proc, lnf);
        pages[2] = std::make_unique<HoloroomPage> (proc, lnf);   // v0.44 TOUCH
        pages[3] = std::make_unique<GrabPage> (proc, lnf);
        pages[4] = std::make_unique<ReelPage> (proc, lnf);
        pages[5] = std::make_unique<DialPage> (proc, lnf);
        pages[6] = std::make_unique<WarpPage> (proc, lnf);
        pages[7] = std::make_unique<DrawAutoPage> (proc, lnf);   // v0.45: DRAW automation (DOODLE moves to EVOLVE)
        pages[8] = std::make_unique<BossPage> (proc, lnf);
        pages[9] = std::make_unique<LiquidPage> (proc, lnf);
        pages[10] = std::make_unique<IntentPage> (proc, lnf);
        pages[11] = std::make_unique<ErosionPage> (proc, lnf);
        pages[12] = std::make_unique<ChainsPage> (proc, lnf);
        pages[13] = std::make_unique<ListenPage> (proc, lnf);
        pages[14] = std::make_unique<ClubPage> (proc, lnf);       // v0.45 WORLDS
        pages[15] = std::make_unique<SeasoningPage> (proc, lnf);
        pages[16] = std::make_unique<EnginePage> (proc, lnf);
        auto mood = std::make_unique<MoodWordsPage> (proc, lnf);
        mood->onOpenPage = [this] (int i) { show (i); };
        moodPage = mood.get();
        pages[17] = std::move (mood);
        for (auto& pg : pages) addChildComponent (*pg);
        static const char* names[] { "MIX LAB", "FX RACK", "HOLOROOM", "GRAB", "REMIX REEL", "DIAL-UP", "WARP DRIVE", "DRAW", "FINAL BOSS", "LIQUID", "INTENT", "EROSION", "FEED", "NEURAL EAR",
                                     "CLUB", "SEASONING", "ENGINE", "MOOD WORDS" };
        static const char* tips[] { "EQ, compressor, vintage colour, space + echo - and the COACH", "SURPRISE FX, STEP FX and the RACK",
                                    "the sound is a glowing orb in a 3D room - drag it near, far, left, right, up, down", "grab the living spectrum: pull it, push it, squeeze it, tear it",
                                    "a 16-step reel that re-cuts the music: slices, loops, stops, filters", "old phones, voice notes, bad signal, walkie-talkies",
                                    "octaves, chipmunks, demons, alien frequency shifts", "draw a curve over the bars - it moves the filter, space, drive, width or volume",
                                    "the master's last stage: LUFS loudness + PEAK SAFE", "the kick carves its hole in the bass - the bass flows around it",
                                    "one breath moves many muscles: rasp, width, filter, frozen air", "push it and it tires, starve it and it sinks into rumble",
                                    "ready-made chains of effects, and your own", "the melody on this track becomes MIDI",
                                    "your track plays in a club: disco ball, strobe, crowd, the DROP", "shake salt, pepper, chilli, sugar, ice, smoke over the track",
                                    "rev it, shift the gears, pick the exhaust, hold TURBO", "type how it should feel - the modules arrange themselves" };
        for (int i = 0; i < numPages; ++i)
        {
            tabs[(size_t) i] = std::make_unique<HotButton> (lnf, names[i]);
            tabs[(size_t) i]->framed = true; tabs[(size_t) i]->setTooltip (tips[i]);
            tabs[(size_t) i]->onClick = [this, i] { show (i); };
            addChildComponent (*tabs[(size_t) i]);
        }
        static const char* doorNames[] { "MIX", "TOUCH", "ORGANIC", "WORLDS", "ERAS", "EAR" };
        static const char* doorTips[] { "MIX LAB, FX RACK and FEED - the mixing desk", "HOLOROOM, GRAB and DRAW - shape it with your hand",
                                        "LIQUID, INTENT and EROSION - the sound behaves like a living thing", "CLUB, SEASONING, ENGINE and MOOD WORDS - play it in other worlds",
                                        "REMIX REEL, DIAL-UP, WARP DRIVE and FINAL BOSS - old tech, the internet, the future", "NEURAL EAR - the melody becomes MIDI" };
        for (int d = 0; d < numDoors; ++d)
        {
            doors[(size_t) d] = std::make_unique<HotButton> (lnf, doorNames[d]);
            doors[(size_t) d]->framed = true; doors[(size_t) d]->setTooltip (doorTips[d]);
            doors[(size_t) d]->onClick = [this, d] { show (lastSub[(size_t) d]); };
            addAndMakeVisible (*doors[(size_t) d]);
            lastSub[(size_t) d] = doorPages (d).front();
        }
        themeBtn.framed = true; themeBtn.setButtonText (kk::theme().night ? "DAY" : "NIGHT"); themeBtn.setTooltip ("Day / night");
        themeBtn.onClick = [this]
        {
            kk::themeIndex() = 1 - kk::themeIndex(); ++kk::themeVersion();
            if (auto s = openSettings()) s->setValue ("theme", kk::themeIndex());
            lnf.setSkin (Skin::all()[(size_t) kk::themeIndex()]);
            if (auto* c = activeLabCache()) c->rebuild();
            themeBtn.setButtonText (kk::theme().night ? "DAY" : "NIGHT");
            if (auto* ed = findParentComponentOfClass<KeysKillaEditor>()) ed->themeChanged();
            repaint(); for (auto& pg : pages) pg->repaint();
        };
        addAndMakeVisible (themeBtn);
        addMouseListener (this, true);   // v0.44 SHAKE: the mouse is watched over the whole window
        shown = jlimit (0.0f, 1.0f, proc.shake.energy.load());
        show (0);
        startTimerHz (30);
    }
    ~FxMainPage() override { removeMouseListener (this); for (auto& pg : pages) pg.reset(); for (auto& tb : tabs) tb.reset(); for (auto& d : doors) d.reset(); setLookAndFeel (nullptr); }
    void showView (int v) { show (jlimit (0, numPages - 1, v)); }
    int currentPage() const { return current; }
    void debugMood (const String& words) { if (moodPage != nullptr) moodPage->debugType (words); }   // test snapshots
    void paint (Graphics& g) override
    {
        pageBackdrop (g, *this, 0.75f);
        const auto& t = kk::theme();
        g.setColour (t.text); g.setFont (kk::modern::font (26.0f, true, 0.3f));
        g.drawFittedText ("EVOLVE", Rectangle<int> (22, 20, 156, 36), Justification::centredLeft, 1, 0.8f);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (16.0f, true, 0.2f));
        g.drawText ("FX PRO", 24, 56, 90, 22, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.12f));
        g.drawText ("by TrapVST", 24, 78, 140, 16, Justification::centredLeft);
        paintShake (g);
        // under every door: what is behind it
        static const char* inside[] { "mix lab . fx rack . feed", "holoroom . grab . draw", "liquid . intent . erosion", "club . seasoning . engine . mood words", "remix reel . dial-up . warp . final boss", "neural ear" };
        for (int d = 0; d < numDoors; ++d)
        {
            const auto r = doors[(size_t) d]->getBounds();
            g.setColour (d == doorOf (current) ? kk::accentText().withAlpha (0.85f) : t.dim.withAlpha (0.8f)); g.setFont (kk::modern::font (9.5f, true, 0.04f));
            g.drawFittedText (inside[d], Rectangle<int> (r.getX() + 2, r.getBottom() + 1, r.getWidth() - 4, 24), Justification::centredTop, 2, 0.8f);
        }
        // the strip of the open door
        if (doorPages (doorOf (current)).size() > 1)
        {
            const auto s = stripArea().toFloat();
            g.setColour (t.text.withAlpha (0.05f)); g.fillRoundedRectangle (s, 10);
            g.setColour (t.dim.withAlpha (0.8f)); g.setFont (kk::modern::font (10.0f, true, 0.3f));
            static const char* doorNames[] { "MIX", "TOUCH", "ORGANIC", "WORLDS", "ERAS", "EAR" };
            g.drawText (doorNames[doorOf (current)], s.withWidth (96).reduced (12, 0), Justification::centredLeft);
        }
    }
    void resized() override
    {
        int y = shakeArea().getBottom() + 24;
        for (int d = 0; d < numDoors; ++d) { doors[(size_t) d]->setBounds (18, y, 150, 48); y += 48 + 40; }
        themeBtn.setBounds (18, getHeight() - 62, 150, 40);
        const auto& dp = doorPages (doorOf (current));
        const bool strip = dp.size() > 1;
        const auto S = stripArea();
        int x = S.getX() + 96;
        for (int i = 0; i < numPages; ++i) tabs[(size_t) i]->setVisible (false);
        if (strip) for (int i : dp) { tabs[(size_t) i]->setBounds (x, S.getY() + 4, 150, S.getHeight() - 8); tabs[(size_t) i]->setVisible (true); x += 156; }
        for (auto& pg : pages) pg->setBounds (strip ? Rectangle<int> (184, S.getBottom() + 4, getWidth() - 196, getHeight() - S.getBottom() - 16) : Rectangle<int> (184, 12, getWidth() - 196, getHeight() - 24));
    }
    int preferredScale() const { return jlimit (50, 100, openSettings()->getIntValue ("fxScale", 80)); }
    // v0.44 SHAKE: fast reversals of the hovering mouse charge the glitch energy
    void mouseMove (const MouseEvent& e) override
    {
        if (e.eventTime == lastMoveTime) return;   // this component hears its own moves twice (as itself and as its own listener)
        lastMoveTime = e.eventTime;
        const auto p = e.getScreenPosition().toFloat();
        const double now = e.eventTime.toMilliseconds();
        for (int ax = 0; ax < 2; ++ax)
        {
            auto& a = axes[ax];
            const float v = ax == 0 ? p.x : p.y, d = v - a.last;
            a.last = v;
            if (std::abs (d) < 1.5f) continue;
            const int dir = d > 0 ? 1 : -1;
            a.travel += std::abs (d);
            if (a.dir != 0 && dir != a.dir)
            {
                if (a.travel > 14.0f && now - a.lastRev < 220.0) proc.shake.charge (jlimit (0.04f, 0.16f, a.travel / 900.0f + 0.04f));
                a.lastRev = now; a.travel = 0;
            }
            a.dir = dir;
        }
    }
private:
    struct Axis { float last = 0, travel = 0; int dir = 0; double lastRev = 0; };
    Rectangle<int> shakeArea() const { return { 18, 102, 150, 44 }; }
    Rectangle<int> stripArea() const { return { 184, 10, getWidth() - 196, 44 }; }
    void paintShake (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto r = shakeArea().toFloat();
        const float e = shown;
        const Colour c (0xffff4fd8), c2 (0xff3ee8ff);
        auto pill = r.withHeight (24.0f);
        if (e > 0.01f) { g.setGradientFill (ColourGradient (c.withAlpha (0.6f * e), pill.getCentreX(), pill.getCentreY(), c.withAlpha (0.0f), pill.getRight() + 10, pill.getCentreY(), true)); g.fillRoundedRectangle (pill.expanded (8, 6), 14); }
        g.setColour (c.withAlpha (0.12f + 0.35f * e)); g.fillRoundedRectangle (pill, 12);
        g.setColour (c.interpolatedWith (c2, e).withAlpha (0.5f + 0.5f * e)); g.drawRoundedRectangle (pill.reduced (0.5f), 12, 1.0f + e);
        // the word itself trembles with the energy
        Random jr ((int64) (phaseShake * 100.0f));
        const float jx = e * 3.0f * (jr.nextFloat() - 0.5f), jy = e * 2.0f * (jr.nextFloat() - 0.5f);
        g.setColour ((t.night ? Colours::white : t.text).withAlpha (0.55f + 0.45f * e)); g.setFont (kk::modern::font (12.0f, true, 0.45f));
        g.drawText ("SHAKE", pill.withTrimmedLeft (14).translated (jx, jy), Justification::centredLeft);
        for (int k = 0; k < 5; ++k)   // charge dots
        {
            const bool lit = e > (float) k / 5.0f + 0.02f;
            g.setColour (lit ? c.interpolatedWith (c2, (float) k / 4.0f) : t.dim.withAlpha (0.3f));
            g.fillEllipse (pill.getRight() - 46 + (float) k * 7.0f, pill.getCentreY() - 2.5f, 5, 5);
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (9.5f, true, 0.05f));
        g.drawText ("shake the mouse = glitch", r.withTrimmedTop (27).toNearestInt(), Justification::centredLeft);
    }
    void timerCallback() override
    {
        const float e = jlimit (0.0f, 1.0f, proc.shake.energy.load());
        shown = e > shown ? e : shown + (e - shown) * 0.3f;
        if (shown < 0.005f) shown = 0.0f;
        if (shown > 0.0f || lastShown > 0.0f) { phaseShake += 1.0f; repaint (shakeArea().expanded (12, 8)); }
        lastShown = shown;
    }
    void show (int i)
    {
        i = jlimit (0, numPages - 1, i);
        current = i;
        const int door = doorOf (i);
        lastSub[(size_t) door] = i;
        for (int d = 0; d < numDoors; ++d) { doors[(size_t) d]->selected = d == door; doors[(size_t) d]->repaint(); }
        for (int k = 0; k < numPages; ++k) { pages[(size_t) k]->setVisible (k == i); tabs[(size_t) k]->selected = k == i; tabs[(size_t) k]->repaint(); }
        resized();
        repaint();
    }
    KeysKillaProcessor& proc;
    LabCacheHolder cacheHolder;
    KKLookAndFeel lnf;
    std::array<std::unique_ptr<Component>, numPages> pages;
    std::array<std::unique_ptr<HotButton>, numPages> tabs;
    std::array<std::unique_ptr<HotButton>, numDoors> doors;
    std::array<int, numDoors> lastSub {};
    MoodWordsPage* moodPage = nullptr;
    int current = 0;
    HotButton themeBtn { lnf };
    Axis axes[2];
    Time lastMoveTime;
    float shown = 0, lastShown = 0, phaseShake = 0;
};
