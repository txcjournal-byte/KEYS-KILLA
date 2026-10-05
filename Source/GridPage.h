// v0.45 GRID page (included by PluginEditor.cpp): make a sound by clicking squares on graph paper (16 x 10).
// Every square holds a hidden gene; its place means something: left -> right = dark -> bright, top -> bottom = short -> long,
// the faint colour zones = material families.  Up to 10 squares blend into ONE sound.  Under the grid a WAV PLAYER loops it
// all the time and changes with every square (rendered in the background - the UI never waits).  Edit the WAV: drag the
// handles = trim, drag the top corners = fade, REVERSE, LONGER / SHORTER.  Then DRAG WAV into FL, EXPORT, or SAVE to MY SOUNDS.
// No MIDI here: MIDI lives in MELODY (GRID -> MY SOUNDS -> MELODY).
#include "Grid.h"

class GridPage : public Component, private Timer
{
public:
    GridPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (playBtn, "PLAY", "Play the WAV - it loops and changes with every square you click", [this] { setPlaying (! playing); });
        btn (loopBtn, "LOOP", "LOOP on = it plays all the time.  Off = once per PLAY", [this] { looping = ! looping; loopBtn.selected = looping; loopBtn.repaint(); if (playing) push(); });
        btn (revBtn, "REVERSE", "Play it backwards", [this] { edit.reverse = ! edit.reverse; const float s = edit.start; edit.start = 1.0f - edit.end; edit.end = 1.0f - s; std::swap (edit.fadeIn, edit.fadeOut); revBtn.selected = edit.reverse; revBtn.repaint(); reEdit (true); });
        btn (resetBtn, "RESET EDIT", "Back to the whole WAV: no trim, no fade, forwards, its own length", [this] { edit = {}; revBtn.selected = false; revBtn.repaint(); reEdit (true); });
        btn (shorterBtn, "SHORTER", "Squeeze it shorter (it gets higher, like a tape)", [this] { edit.length = jmax (0.25f, edit.length / 1.25f); reEdit (true); });
        btn (longerBtn, "LONGER", "Stretch it longer (it gets lower, like a tape)", [this] { edit.length = jmin (4.0f, edit.length * 1.25f); reEdit (true); });
        btn (exportBtn, "EXPORT", "Save the edited WAV anywhere", [this] { exportWav(); });
        btn (saveBtn, "SAVE", "Save the edited WAV into MY SOUNDS (pick a folder / sound kit) - then use it in MELODY", [this]
        {
            auto snd = editedSound();
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<GridPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
        });
        btn (clearBtn, "CLEAR", "Empty the paper", [this] { picks.clear(); changed(); });
        btn (diceBtn, "SURPRISE", "A few random squares", [this]
        {
            picks.clear(); auto& r = Random::getSystemRandom();
            const int n = 3 + r.nextInt (5);
            while ((int) picks.size() < n) { const int i = r.nextInt (kk::grid::cols * kk::grid::rows); if (std::find (picks.begin(), picks.end(), i) == picks.end()) picks.push_back (i); }
            changed();
        });
        loopBtn.selected = true; playBtn.hero = true;
        dragWav.makeFile = [this] { auto s = editedSound(); return s != nullptr ? kk::PairLab::exportWav (*s, rate(), "GRID") : File(); };
        dragWav.setTooltip ("Drag the edited WAV into FL");
        addAndMakeVisible (dragWav);
        startTimerHz (30);
    }
    ~GridPage() override { stopTimer(); worker.stopThread (4000); }

    void visibilityChanged() override
    {
        if (! isVisible() && playing) setPlaying (false);
    }
    // the snapshot: 10 squares and a rendered, edited WAV (rendered here, synchronously)
    void debugPick()
    {
        picks = { kk::grid::cellIndex (2, 1), kk::grid::cellIndex (3, 2), kk::grid::cellIndex (5, 4), kk::grid::cellIndex (7, 3), kk::grid::cellIndex (9, 6),
                  kk::grid::cellIndex (11, 5), kk::grid::cellIndex (12, 7), kk::grid::cellIndex (13, 2), kk::grid::cellIndex (6, 8), kk::grid::cellIndex (14, 8) };
        recipe = kk::grid::blend (picks); genome = makeGenome (recipe);
        raw = proc.renderGenomeAudio (genome, rate(), seconds); rendered = ++jobSent; rawPeaks = kk::grid::peaks (raw, 540);
        edit.start = 0.04f; edit.end = 0.86f; edit.fadeOut = 0.22f; edit.fadeIn = 0.03f;
        reEdit (false);
        hoverCell = kk::grid::cellIndex (11, 5);
        note = "PLAY = it loops, and every square you click changes it";
        repaint();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("GRID", 24, 12, 120, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("click up to 10 squares - they blend into one sound.  left = dark, right = bright, top = short, bottom = long, the colours are the materials",
                    124, 18, getWidth() - 148, 28, Justification::centredLeft);
        drawPaper (g);
        drawPanel (g);
        drawPlayer (g);
    }
    void resized() override
    {
        const auto pa = panelArea();
        clearBtn.setBounds (pa.getRight() - 236, pa.getY() + 12, 110, 36); diceBtn.setBounds (pa.getRight() - 120, pa.getY() + 12, 110, 36);
        const auto c = controlsArea();
        const int w = (c.getWidth() - 20) / 3, h = 42;
        auto row = [&] (int i, Component& a, Component& b, Component& d) { const int y = c.getY() + i * (h + 10); a.setBounds (c.getX(), y, w, h); b.setBounds (c.getX() + w + 10, y, w, h); d.setBounds (c.getX() + 2 * (w + 10), y, w, h); };
        row (0, playBtn, loopBtn, revBtn); row (1, shorterBtn, longerBtn, resetBtn); row (2, exportBtn, saveBtn, dragWav);
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int c = cellAt (e.position);
        if (c != hoverCell) { hoverCell = c; repaint(); }
        setMouseCursor (handleAt (e.position) > 0 ? MouseCursor::LeftRightResizeCursor : c >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }
    void mouseExit (const MouseEvent&) override { if (hoverCell >= 0) { hoverCell = -1; repaint(); } }
    void mouseDown (const MouseEvent& e) override
    {
        if (const int c = cellAt (e.position); c >= 0)
        {
            if (auto it = std::find (picks.begin(), picks.end(), c); it != picks.end()) picks.erase (it);
            else if ((int) picks.size() >= kk::grid::maxPicks) { note = "10 squares at most - click one again to free it"; repaint(); return; }
            else picks.push_back (c);
            inkAt = c; ink = 1.0f;
            changed();
            return;
        }
        dragMode = handleAt (e.position);
        if (dragMode == 0 && waveArea().toFloat().contains (e.position) && raw.getNumSamples() > 0) { if (auto s = editedSound()) proc.useSample (s, true); }   // click = hear it once
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragMode == 0) return;
        const auto w = waveArea().toFloat();
        const float x = jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth());
        const float span = jmax (0.02f, edit.end - edit.start);
        if (dragMode == 1) edit.start = jmin (x, edit.end - 0.02f);
        else if (dragMode == 2) edit.end = jmax (x, edit.start + 0.02f);
        else if (dragMode == 3) edit.fadeIn = jlimit (0.0f, 1.0f - edit.fadeOut, (x - edit.start) / span);
        else if (dragMode == 4) edit.fadeOut = jlimit (0.0f, 1.0f - edit.fadeIn, (edit.end - x) / span);
        const auto now = Time::getMillisecondCounter();
        reEdit (now - lastLive > 150);
        if (now - lastLive > 150) lastLive = now;
    }
    void mouseUp (const MouseEvent&) override { if (dragMode != 0) { dragMode = 0; reEdit (true); } }

private:
    using Genome = KeysKillaProcessor::Genome;
    // ---- the background renderer: its own light engine, so the page never waits and never touches the processor's renderer
    struct Renderer : public Thread
    {
        Renderer() : Thread ("EVOLVE GRID render") {}
        std::unique_ptr<KeysKillaProcessor> engine;
        CriticalSection lock; Genome job; int jobId = 0, doneId = 0; double rate = 44100, secs = 2.2;
        std::function<void (int, AudioBuffer<float>)> done;
        void run() override
        {
            while (! threadShouldExit())
            {
                Genome g; int id; double r, s;
                { const ScopedLock sl (lock); g = job; id = jobId; r = rate; s = secs; }
                if (id == doneId || ! g.valid()) { wait (-1); continue; }
                auto buf = engine->renderGenomeAudio (g, r, s);
                doneId = id;
                if (threadShouldExit()) return;
                bool stale; { const ScopedLock sl (lock); stale = jobId != id; }
                if (! stale && done) done (id, std::move (buf));
            }
        }
    };
    double rate() const { return proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0; }
    Rectangle<int> paperArea() const { return { 64, 92, kk::grid::cols * cell, kk::grid::rows * cell }; }
    Rectangle<int> panelArea() const { const auto p = paperArea(); return { p.getRight() + 40, 64, getWidth() - p.getRight() - 40 - 24, p.getBottom() + 20 - 64 }; }
    Rectangle<int> playerArea() const { return { 24, paperArea().getBottom() + 36, getWidth() - 48, getHeight() - paperArea().getBottom() - 36 - 14 }; }
    Rectangle<int> controlsArea() const { const auto p = playerArea(); return { p.getRight() - 18 - 480, p.getY() + 50, 480, p.getHeight() - 62 }; }
    Rectangle<int> waveArea() const { const auto p = playerArea(), c = controlsArea(); return { p.getX() + 18, p.getY() + 50, c.getX() - 24 - p.getX() - 18, p.getHeight() - 92 }; }
    int cellAt (Point<float> p) const
    {
        const auto a = paperArea().toFloat();
        if (! a.contains (p)) return -1;
        return kk::grid::cellIndex (jlimit (0, kk::grid::cols - 1, (int) ((p.x - a.getX()) / (float) cell)), jlimit (0, kk::grid::rows - 1, (int) ((p.y - a.getY()) / (float) cell)));
    }
    // 1 start, 2 end, 3 fade in corner, 4 fade out corner
    int handleAt (Point<float> p) const
    {
        const auto w = waveArea().toFloat();
        if (raw.getNumSamples() == 0 || ! w.expanded (12, 6).contains (p)) return 0;
        const float xs = w.getX() + edit.start * w.getWidth(), xe = w.getX() + edit.end * w.getWidth(), span = xe - xs;
        const float xi = xs + edit.fadeIn * span, xo = xe - edit.fadeOut * span;
        if (p.y < w.getY() + 26)
        {
            if (std::abs (p.x - xi) < 14) return 3;
            if (std::abs (p.x - xo) < 14) return 4;
        }
        if (std::abs (p.x - xs) < 12) return 1;
        if (std::abs (p.x - xe) < 12) return 2;
        return 0;
    }
    Genome makeGenome (const kk::grid::Recipe& r)
    {
        auto g = proc.sculpt (proc.alchemy (r.exc, r.body, r.matter, r.size, r.seed), r.stretch, r.bright, r.heat, r.cool, r.split);
        if (g.valid()) g.name = r.name;
        return g;
    }
    void changed()
    {
        if (picks.empty()) { genome = {}; recipe = {}; raw.setSize (0, 0); edited.setSize (0, 0); rawPeaks.clear(); if (playing) setPlaying (false); repaint(); return; }
        recipe = kk::grid::blend (picks);
        genome = makeGenome (recipe);
        note.clear();
        requestRender();
        repaint();
    }
    void requestRender()
    {
        if (! genome.valid()) return;
        if (worker.engine == nullptr)
        {
            worker.engine = std::make_unique<KeysKillaProcessor> (false);
            worker.engine->renderGenomeAudio (genome, rate(), 0.02);   // warm up here: its own offline renderer is made on this thread
            worker.done = [safe = SafePointer<GridPage> (this)] (int id, AudioBuffer<float> b)
            {
                auto shared = std::make_shared<AudioBuffer<float>> (std::move (b));
                MessageManager::callAsync ([safe, id, shared] { if (safe != nullptr) safe->arrived (id, *shared); });
            };
        }
        { const ScopedLock sl (worker.lock); worker.job = genome; worker.jobId = ++jobSent; worker.rate = rate(); worker.secs = seconds; }
        if (! worker.isThreadRunning()) worker.startThread (Thread::Priority::low);
        worker.notify();
    }
    void arrived (int id, const AudioBuffer<float>& b)
    {
        if (id != jobSent) return;   // an older sound - a newer one is on its way
        raw = b; rendered = id; rawPeaks = kk::grid::peaks (raw, 540); morph = 1.0f;
        reEdit (true);
        if (! playing && firstSound) { firstSound = false; setPlaying (true); }   // the first square starts the player
    }
    void reEdit (bool pushNow)
    {
        edited = kk::grid::apply (raw, edit);
        editedSnd = nullptr;
        viewPeaks = kk::grid::peaks (raw, 540, edit.reverse);
        if (pushNow && playing) push();
        repaint();
    }
    kk::PairPtr editedSound()
    {
        if (edited.getNumSamples() == 0) return nullptr;
        if (editedSnd == nullptr) editedSnd = kk::PairLab::fromBuffer (edited, rate(), rate(), genome.valid() ? genome.name : String ("Grid sound"));
        return editedSnd;
    }
    void push()
    {
        auto s = editedSound();
        if (s == nullptr) return;
        proc.useSample (s, ! looping);
        if (looping)
        {
            const double bpm = jlimit (40.0, 300.0, proc.lastBpm.load());
            const double beats = (double) s->audio.getNumSamples() / rate() * bpm / 60.0;
            proc.playCustomLoop ({ { 0.0f, (float) beats * 0.995f, s->rootNote, false } }, beats);
            loopPeriod = jmax (1.0, beats) * 60.0 / bpm;
        }
        playStart = Time::getMillisecondCounterHiRes();
    }
    void setPlaying (bool on)
    {
        playing = on && edited.getNumSamples() > 0;
        if (playing) push();
        else if (proc.loopOwnerId() == 4) proc.stopLoop();
        playBtn.setButtonText (playing ? "STOP" : "PLAY"); playBtn.selected = playing; playBtn.repaint();
        repaint();
    }
    void exportWav()
    {
        if (edited.getNumSamples() == 0) { note = "click some squares first"; repaint(); return; }
        const auto name = File::createLegalFileName (genome.valid() ? genome.name : String ("Grid sound"));
        chooser = std::make_unique<FileChooser> ("EXPORT WAV", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (name + ".wav"), "*.wav");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                              [safe = SafePointer<GridPage> (this), buf = edited, r = rate()] (const FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == File()) return;
            f = f.withFileExtension (".wav");
            const bool ok = kk::grid::writeWav (f, buf, r);
            if (safe != nullptr) { safe->note = ok ? "exported: " + f.getFileName() : String ("could not write the file"); safe->repaint(); }
        });
    }

    // ---- drawing
    void drawPaper (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = paperArea().toFloat();
        const auto& Z = kk::grid::zones();
        // the sheet, a little shadow, slightly lifted
        g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (a.expanded (14).translated (3, 6), 6);
        const auto paper = Colour (0xfff4f0e6);
        g.setColour (paper); g.fillRoundedRectangle (a.expanded (14), 6);
        // faint colour zones = material families
        for (int i = 0; i < kk::grid::cols * kk::grid::rows; ++i)
        {
            const auto r = Rectangle<float> (a.getX() + (float) kk::grid::cellCol (i) * cell, a.getY() + (float) kk::grid::cellRow (i) * cell, (float) cell, (float) cell);
            g.setColour (Colour (Z[(size_t) kk::grid::zoneOf (i)].colour).withAlpha (0.2f)); g.fillRect (r);
        }
        // graph lines: fine every square, a stronger one every 4
        for (int c = 0; c <= kk::grid::cols; ++c) { g.setColour (Colour (0xff3a78c8).withAlpha (c % 4 == 0 ? 0.55f : 0.25f)); g.drawLine (a.getX() + (float) (c * cell), a.getY(), a.getX() + (float) (c * cell), a.getBottom(), c % 4 == 0 ? 1.4f : 0.8f); }
        for (int r = 0; r <= kk::grid::rows; ++r) { g.setColour (Colour (0xff3a78c8).withAlpha (r % 5 == 0 ? 0.55f : 0.25f)); g.drawLine (a.getX(), a.getY() + (float) (r * cell), a.getRight(), a.getY() + (float) (r * cell), r % 5 == 0 ? 1.4f : 0.8f); }
        // zone names, hand-written in the margin of each zone
        g.setFont (kk::modern::font (11.0f, true, 0.3f));
        for (auto& z : Z) { g.setColour (Colour (z.colour).darker (0.9f).withAlpha (0.55f)); g.drawText (z.name, Rectangle<float> (a.getX() + z.cx * (a.getWidth() - cell) - 30 + cell * 0.5f, a.getY() + z.cy * (a.getHeight() - cell) + cell * 0.5f - 8, 60 + cell, 16).toNearestInt(), Justification::centredLeft); }
        // the chosen squares: inked with their family, numbered in the order you picked them
        for (int k = 0; k < (int) picks.size(); ++k)
        {
            const int i = picks[(size_t) k];
            const auto r = Rectangle<float> (a.getX() + (float) kk::grid::cellCol (i) * cell, a.getY() + (float) kk::grid::cellRow (i) * cell, (float) cell, (float) cell).reduced (3);
            const auto zc = Colour (Z[(size_t) kk::grid::zoneOf (i)].colour);
            const float pop = i == inkAt ? 1.0f + 0.25f * ink : 1.0f;
            const auto rr = r.withSizeKeepingCentre (r.getWidth() * pop, r.getHeight() * pop);
            g.setColour (zc.withAlpha (0.35f)); g.fillRoundedRectangle (rr.expanded (3), 7);
            g.setGradientFill (ColourGradient (zc.brighter (0.3f), rr.getX(), rr.getY(), zc.darker (0.5f), rr.getRight(), rr.getBottom(), false));
            g.fillRoundedRectangle (rr, 5);
            const float breathe = playing ? 0.5f + 0.5f * std::sin (phase * 4.0f + (float) k) : 0.6f;
            g.setColour (Colours::white.withAlpha (0.4f + 0.4f * breathe)); g.drawRoundedRectangle (rr, 5, 1.5f);
            g.setColour (Colour (0xff1b1d2a)); g.setFont (kk::modern::font (13.0f, true, 0.0f));
            g.drawText (String (k + 1), rr.toNearestInt(), Justification::centred);
        }
        // lines between the chosen squares: they are one sound
        if (picks.size() > 1)
        {
            Path p;
            for (int k = 0; k < (int) picks.size(); ++k)
            {
                const auto pt = Point<float> (a.getX() + ((float) kk::grid::cellCol (picks[(size_t) k]) + 0.5f) * cell, a.getY() + ((float) kk::grid::cellRow (picks[(size_t) k]) + 0.5f) * cell);
                if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            g.setColour (Colour (0xff1b1d2a).withAlpha (0.35f));
            const float dash[] { 4.0f, 4.0f };
            PathStrokeType (1.3f).createDashedStroke (p, p, dash, 2);
            g.fillPath (p);
        }
        if (hoverCell >= 0)
        {
            const auto r = Rectangle<float> (a.getX() + (float) kk::grid::cellCol (hoverCell) * cell, a.getY() + (float) kk::grid::cellRow (hoverCell) * cell, (float) cell, (float) cell);
            g.setColour (Colour (0xff1b1d2a).withAlpha (0.6f)); g.drawRect (r, 2.0f);
        }
        // what the axes mean
        g.setColour (t.text.withAlpha (0.8f)); g.setFont (kk::modern::font (12.0f, true, 0.3f));
        g.drawText ("DARK", (int) a.getX(), (int) a.getY() - 34, 120, 16, Justification::centredLeft);
        g.drawText ("BRIGHT", (int) a.getRight() - 120, (int) a.getY() - 34, 120, 16, Justification::centredRight);
        g.setColour (t.text.withAlpha (0.35f)); g.drawArrow (Line<float> (a.getX() + 52, a.getY() - 26, a.getRight() - 70, a.getY() - 26), 1.2f, 7, 9);
        {
            Graphics::ScopedSaveState ss (g);
            g.addTransform (AffineTransform::rotation (-MathConstants<float>::halfPi, a.getX() - 30, a.getCentreY()));
            g.setColour (t.text.withAlpha (0.8f));
            g.drawText ("LONG", (int) (a.getX() - 30 - a.getHeight() * 0.5f), (int) a.getCentreY() - 8, 120, 16, Justification::centredLeft);
            g.drawText ("SHORT", (int) (a.getX() - 30 + a.getHeight() * 0.5f) - 120, (int) a.getCentreY() - 8, 120, 16, Justification::centredRight);
            g.setColour (t.text.withAlpha (0.35f)); g.drawArrow (Line<float> (a.getX() - 30 + a.getHeight() * 0.5f - 64, a.getCentreY(), a.getX() - 30 - a.getHeight() * 0.5f + 56, a.getCentreY()), 1.2f, 7, 9);
        }
    }
    void drawPanel (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = panelArea().toFloat();
        kk::modern::well (g, a, 16.0f);
        auto r = a.reduced (22, 16);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
        g.drawText ("THE SOUND", r.removeFromTop (24).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.08f));
        g.drawText (String ((int) picks.size()) + " of 10 squares", r.removeFromTop (18).toNearestInt(), Justification::centredLeft);
        r.removeFromTop (14);
        if (genome.valid())
        {
            g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.03f));
            g.drawFittedText (genome.name, r.removeFromTop (40).toNearestInt(), Justification::centredLeft, 1, 0.7f);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.12f));
            g.drawText (String (kk::alc::exciters()[(size_t) recipe.exc].name) + "  x  " + kk::alc::bodies()[(size_t) recipe.body].name + "   ·   " + kk::alc::matterWord (recipe.matter).toUpperCase(),
                        r.removeFromTop (20).toNearestInt(), Justification::centredLeft);
            r.removeFromTop (12);
            StringArray tags;
            tags.add (recipe.bright < -0.35f ? "DARK" : recipe.bright > 0.35f ? "BRIGHT" : "WARM");
            tags.add (recipe.stretch < -0.35f ? "SHORT" : recipe.stretch > 0.35f ? "LONG" : "MEDIUM");
            if (recipe.heat > 0.15f) tags.add ("HOT"); if (recipe.cool > 0.15f) tags.add ("FROZEN"); if (recipe.split > 0.1f) tags.add ("LAYERED");
            auto tr = r.removeFromTop (30); float x = tr.getX();
            g.setFont (kk::modern::font (12.0f, true, 0.2f));
            for (auto& s : tags)
            {
                const auto pill = Rectangle<float> (x, tr.getY(), 96, 28);
                g.setColour (t.accent.withAlpha (0.16f)); g.fillRoundedRectangle (pill, 14);
                g.setColour (t.accent.withAlpha (0.8f)); g.drawRoundedRectangle (pill.reduced (0.5f), 14, 1.0f);
                g.setColour (kk::accentText()); g.drawText (s, pill.toNearestInt(), Justification::centred);
                x += 104;
            }
        }
        else
        {
            g.setColour (t.text.withAlpha (0.8f)); g.setFont (kk::modern::font (24.0f, true, 0.03f));
            g.drawText ("click a square", r.removeFromTop (40).toNearestInt(), Justification::centredLeft);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.06f));
            g.drawText ("every square hides a gene - its place says what it does", r.removeFromTop (20).toNearestInt(), Justification::centredLeft);
            r.removeFromTop (42);
        }
        r.removeFromTop (18);
        // the hovered square
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (12.5f, true, 0.25f));
        g.drawText ("THIS SQUARE", r.removeFromTop (20).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.05f));
        String hs = "point at a square";
        if (hoverCell >= 0)
        {
            const auto& z = kk::grid::zones()[(size_t) kk::grid::zoneOf (hoverCell)];
            const float b = kk::grid::brightOf (hoverCell), s = kk::grid::stretchOf (hoverCell);
            hs = String (z.name) + " family   ·   " + (b < -0.3f ? "dark" : b > 0.3f ? "bright" : "warm") + "   ·   " + (s < -0.3f ? "short" : s > 0.3f ? "long" : "medium")
                 + (std::find (picks.begin(), picks.end(), hoverCell) != picks.end() ? "   ·   chosen (click = free it)" : "");
        }
        g.drawText (hs, r.removeFromTop (20).toNearestInt(), Justification::centredLeft);
        r.removeFromTop (16);
        // the families
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (12.5f, true, 0.25f));
        g.drawText ("THE COLOURS", r.removeFromTop (20).toNearestInt(), Justification::centredLeft);
        r.removeFromTop (6);
        const auto& Z = kk::grid::zones();
        const float colW = r.getWidth() / 2;
        for (int k = 0; k < (int) Z.size(); ++k)
        {
            const auto cr = Rectangle<float> (r.getX() + (float) (k % 2) * colW, r.getY() + (float) (k / 2) * 26.0f, colW - 8, 22);
            g.setColour (Colour (Z[(size_t) k].colour)); g.fillRoundedRectangle (cr.withWidth (22).reduced (2), 4);
            g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (11.5f, true, 0.12f));
            g.drawText (String (Z[(size_t) k].name) + "  " + String::fromUTF8 ("·") + "  " + String (kk::alc::exciters()[(size_t) Z[(size_t) k].exc].word).toLowerCase() + " / " + String (kk::alc::bodies()[(size_t) Z[(size_t) k].body].word).toLowerCase(),
                        cr.withTrimmedLeft (30).toNearestInt(), Justification::centredLeft);
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.04f));
        g.drawFittedText ("no MIDI here: SAVE the WAV to MY SOUNDS, then make it the melody sound in MELODY - that is where the MIDI loops are",
                          a.reduced (22, 14).withTrimmedTop (a.getHeight() - 52).toNearestInt(), Justification::bottomLeft, 2, 0.9f);
    }
    void drawPlayer (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto p = playerArea().toFloat();
        kk::modern::well (g, p, 16.0f);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
        g.drawText ("WAV PLAYER", (int) p.getX() + 18, (int) p.getY() + 14, 160, 22, Justification::centredLeft);
        const double secs = edited.getNumSamples() / rate();
        String info;
        if (edited.getNumSamples() > 0)
        {
            info << String (secs, 1) << " s";
            if (edit.length > 1.01f) info << "   ·   STRETCHED LONGER"; else if (edit.length < 0.99f) info << "   ·   SQUEEZED SHORTER";
            if (edit.reverse) info << "   ·   REVERSED";
            if (edit.start > 0.001f || edit.end < 0.999f) info << "   ·   TRIMMED";
            if (edit.fadeIn > 0.001f || edit.fadeOut > 0.001f) info << "   ·   FADED";
            if (playing) info << (looping ? "   ·   LOOPING" : "   ·   PLAYING");
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.1f));
        g.drawText (rendered != jobSent ? String ("listening to the squares ...") : info, (int) p.getX() + 180, (int) p.getY() + 15, (int) (controlsArea().getX() - p.getX() - 200), 20, Justification::centredLeft);
        const auto w = waveArea().toFloat();
        g.setColour (Colour (0xff05070c).withAlpha (t.night ? 0.75f : 0.85f)); g.fillRoundedRectangle (w.expanded (6), 10);
        if (viewPeaks.empty() || raw.getNumSamples() == 0)
        {
            g.setColour (Colours::white.withAlpha (0.4f)); g.setFont (kk::modern::font (14.0f, true, 0.06f));
            g.drawText (picks.empty() ? "the WAV appears here when you click a square" : "listening to the squares ...", w.toNearestInt(), Justification::centred);
            return;
        }
        const auto zc = picks.empty() ? t.accent : Colour (kk::grid::zones()[(size_t) kk::grid::zoneOf (picks.front())].colour);
        const float xs = w.getX() + edit.start * w.getWidth(), xe = w.getX() + edit.end * w.getWidth(), span = xe - xs;
        const float xi = xs + edit.fadeIn * span, xo = xe - edit.fadeOut * span;
        const int n = (int) viewPeaks.size();
        const float cy = w.getCentreY(), hh = w.getHeight() * 0.46f;
        float mx = 0.001f; for (auto v : viewPeaks) mx = jmax (mx, v);
        const float bw = w.getWidth() / (float) n;
        for (int k = 0; k < n; ++k)
        {
            const float x = w.getX() + (float) k * bw;
            float v = viewPeaks[(size_t) k] / mx * (0.97f - 0.12f * morph);
            const bool in = x >= xs && x <= xe;
            float env = 1.0f;
            if (in && x < xi && xi > xs) env = (x - xs) / (xi - xs);
            if (in && x > xo && xe > xo) env = jmin (env, (xe - x) / (xe - xo));
            const float h = jmax (0.6f, v * hh);
            g.setColour (in ? zc.interpolatedWith (Colours::white, 0.25f * (float) k / (float) n).withAlpha (0.35f + 0.6f * env) : Colours::white.withAlpha (0.12f));
            g.fillRect (x, cy - h, jmax (1.0f, bw - 0.5f), h * 2);
        }
        // trimmed away: dimmed
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillRect (w.withRight (xs)); g.fillRect (w.withLeft (xe));
        // the fade envelope
        Path env; env.startNewSubPath (xs, w.getBottom()); env.lineTo (xi, w.getY() + 4); env.lineTo (xo, w.getY() + 4); env.lineTo (xe, w.getBottom());
        g.setColour (Colours::white.withAlpha (0.7f)); g.strokePath (env, PathStrokeType (1.4f));
        for (auto fx : { xi, xo }) { g.setColour (Colour (0xffffd23f)); g.fillEllipse (fx - 7, w.getY() - 3, 14, 14); g.setColour (Colours::black.withAlpha (0.6f)); g.drawEllipse (fx - 7, w.getY() - 3, 14, 14, 1.0f); }
        // trim handles
        for (auto hx : { xs, xe })
        {
            g.setColour (Colour (0xff36ff6a)); g.fillRect (hx - 1.5f, w.getY() - 4, 3.0f, w.getHeight() + 8);
            const auto grip = Rectangle<float> (hx - 7, cy - 18, 14, 36);
            g.fillRoundedRectangle (grip, 4);
            g.setColour (Colours::black.withAlpha (0.55f)); for (int k = 0; k < 3; ++k) g.fillRect (grip.getX() + 3, grip.getY() + 11 + k * 6.0f, 8.0f, 1.5f);
        }
        // the play head
        if (playing && loopPeriod > 0)
        {
            const double el = (Time::getMillisecondCounterHiRes() - playStart) / 1000.0;
            const double pos = looping ? std::fmod (el, loopPeriod) / secs : el / secs;
            if (pos <= 1.0)
            {
                const float x = xs + (float) pos * span;
                g.setColour (Colours::white.withAlpha (0.9f)); g.fillRect (x - 1.0f, w.getY(), 2.0f, w.getHeight());
                g.setColour (Colours::white.withAlpha (0.15f)); g.fillRect (x - 6.0f, w.getY(), 12.0f, w.getHeight());
            }
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawText ("drag the green handles = trim      drag the yellow corners = fade      click the wave = hear it once", (int) w.getX(), (int) w.getBottom() + 10, (int) w.getWidth() - 330, 18, Justification::centredLeft);
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.0f, true, 0.03f)); g.drawFittedText (note, (int) w.getRight() - 320, (int) w.getBottom() + 10, 320, 18, Justification::centredRight, 1, 0.8f); }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.04f;
        if (ink > 0) ink = jmax (0.0f, ink - 0.06f);
        if (morph > 0) morph = jmax (0.0f, morph - 0.08f);
        if (playing && ! looping && loopPeriod > 0 && (Time::getMillisecondCounterHiRes() - playStart) / 1000.0 > edited.getNumSamples() / rate() + 0.1) setPlaying (false);
        if (playing || ink > 0 || morph > 0 || rendered != jobSent) repaint();
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton playBtn { lnf }, loopBtn { lnf }, revBtn { lnf }, resetBtn { lnf }, shorterBtn { lnf }, longerBtn { lnf }, exportBtn { lnf }, saveBtn { lnf }, clearBtn { lnf }, diceBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    static constexpr int cell = 40;
    static constexpr double seconds = 2.2;
    std::vector<int> picks;
    kk::grid::Recipe recipe;
    Genome genome;
    AudioBuffer<float> raw, edited;
    kk::PairPtr editedSnd;
    std::vector<float> rawPeaks, viewPeaks;
    kk::grid::Edit edit;
    Renderer worker;
    std::unique_ptr<FileChooser> chooser;
    String note;
    int hoverCell = -1, dragMode = 0, jobSent = 0, rendered = 0, inkAt = -1;
    bool playing = false, looping = true, firstSound = true;
    double playStart = 0, loopPeriod = 1.0;
    uint32 lastLive = 0;
    float phase = 0, ink = 0, morph = 0;
};
