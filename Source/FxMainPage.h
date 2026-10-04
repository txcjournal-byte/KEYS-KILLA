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

class FxMainPage : public Component
{
public:
    static constexpr int numPages = 9;
    explicit FxMainPage (KeysKillaProcessor& p) : proc (p)
    {
        lnf.setSkin (Skin::all()[(size_t) kk::themeIndex()]);
        if (auto* c = activeLabCache(); c != nullptr && c->builtFor != kk::themeIndex()) c->rebuild();
        setLookAndFeel (&lnf);
        pages[0] = std::make_unique<MixLabPage> (proc, lnf);
        pages[1] = std::make_unique<FxRackPage> (proc, lnf);
        pages[2] = std::make_unique<ReelPage> (proc, lnf);
        pages[3] = std::make_unique<DialPage> (proc, lnf);
        pages[4] = std::make_unique<WarpPage> (proc, lnf);
        pages[5] = std::make_unique<DoodlePage> (proc, lnf);
        pages[6] = std::make_unique<BossPage> (proc, lnf);
        pages[7] = std::make_unique<ChainsPage> (proc, lnf);
        pages[8] = std::make_unique<ListenPage> (proc, lnf);
        for (auto& pg : pages) addChildComponent (*pg);
        static const char* names[] { "MIX LAB", "FX RACK", "REMIX REEL", "DIAL-UP", "WARP DRIVE", "DOODLE", "FINAL BOSS", "FEED", "NEURAL EAR" };
        static const char* tips[] { "EQ, compressor, vintage colour, space + echo - and the COACH", "SURPRISE FX, STEP FX and the RACK",
                                    "a 16-step reel that re-cuts the music: slices, loops, stops, filters", "old phones, voice notes, bad signal, walkie-talkies",
                                    "octaves, chipmunks, demons, alien frequency shifts", "draw a line - get a melody as MIDI",
                                    "the master's last stage: LUFS loudness + PEAK SAFE", "ready-made chains of effects, and your own",
                                    "the melody on this track becomes MIDI" };
        for (int i = 0; i < numPages; ++i)
        {
            tabs[(size_t) i] = std::make_unique<HotButton> (lnf, names[i]);
            tabs[(size_t) i]->framed = true; tabs[(size_t) i]->setTooltip (tips[i]);
            tabs[(size_t) i]->onClick = [this, i] { show (i); };
            addAndMakeVisible (*tabs[(size_t) i]);
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
        show (0);
    }
    ~FxMainPage() override { for (auto& pg : pages) pg.reset(); for (auto& tb : tabs) tb.reset(); setLookAndFeel (nullptr); }
    void showView (int v) { show (jlimit (0, numPages - 1, v)); }
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
        // era groups next to the tabs
        static const std::pair<int, const char*> eras[] { { 2, "ERAS" }, { 7, "SMART" } };
        g.setFont (kk::modern::font (9.0f, true, 0.25f));
        for (auto& [i, n] : eras) { g.setColour (t.dim.withAlpha (0.7f)); g.drawText (n, 20, tabs[(size_t) i]->getY() - 13, 150, 11, Justification::centredLeft); }
    }
    void resized() override
    {
        int y = 104;
        for (int i = 0; i < numPages; ++i) { if (i == 2 || i == 7) y += 14; tabs[(size_t) i]->setBounds (18, y, 150, 46); y += 52; }
        themeBtn.setBounds (18, getHeight() - 62, 150, 40);
        for (auto& pg : pages) pg->setBounds (184, 12, getWidth() - 196, getHeight() - 24);
    }
    int preferredScale() const { return jlimit (50, 100, openSettings()->getIntValue ("fxScale", 80)); }
private:
    void show (int i)
    {
        for (int k = 0; k < numPages; ++k) { pages[(size_t) k]->setVisible (k == i); tabs[(size_t) k]->selected = k == i; tabs[(size_t) k]->repaint(); }
    }
    KeysKillaProcessor& proc;
    LabCacheHolder cacheHolder;
    KKLookAndFeel lnf;
    std::array<std::unique_ptr<Component>, numPages> pages;
    std::array<std::unique_ptr<HotButton>, numPages> tabs;
    HotButton themeBtn { lnf };
};
