// v0.45 SOUND DOCK (included by PluginEditor.cpp): the same player on every EVOLVE page, always in the same place above the keys.
// It shows the WAVEFORM of whatever is on the keys now (it follows every change), its name, PLAY / LOOP, trim handles (start / end),
// FADE, REVERSE, DRAG WAV into FL and SAVE to MY SOUNDS. The edits shape the exported WAV - the user never hunts for a player again.
class SoundDock : public Component, public SettableTooltipClient, private Timer
{
public:
    SoundDock (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (playBtn, "PLAY", "Hear the sound on the keys", [this] { proc.previewNote = 60; flash = 1.0f; });
        btn (loopBtn, "LOOP", "Hear it again and again while you shape it anywhere in the plugin", [this] { looping = ! looping; loopBtn.selected = looping; loopBtn.repaint(); });
        btn (revBtn, "REV", "Reverse the WAV you drag / save", [this] { reversed = ! reversed; revBtn.selected = reversed; revBtn.repaint(); repaint(); });
        btn (saveBtn, "SAVE", "Save this sound (with your trim / fade / reverse) into MY SOUNDS", [this]
        {
            auto snd = edited(); if (snd == nullptr) return;
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<SoundDock> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->noteT = 90; safe->repaint(); } });
        });
        dragWav.makeFile = [this] { auto snd = edited(); return snd != nullptr ? writeTemp (*snd) : File(); };
        dragWav.setTooltip ("Drag this sound (with your edits) into FL as a WAV");
        addAndMakeVisible (dragWav);
        setTooltip ("SOUND DOCK: what the keys play now.  Drag the handles = trim, drag the top corners = fade");
        startTimerHz (15);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto r = getLocalBounds().toFloat().reduced (1);
        g.setColour (Colour (0xff0a0b11).withAlpha (0.92f)); g.fillRoundedRectangle (r, 10);
        g.setColour (t.accent.withAlpha (0.25f + 0.5f * flash)); g.drawRoundedRectangle (r, 10, 1.2f);
        // name
        g.setColour (Colours::white); g.setFont (kk::modern::font (13.0f, true, 0.06f));
        g.drawFittedText (name.isEmpty() ? String ("-") : name, nameArea().toNearestInt(), Justification::centredLeft, 2, 0.7f);
        // waveform with trim + fade
        const auto w = waveArea();
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (w, 6);
        if (! peaks.empty())
        {
            const int n = (int) peaks.size();
            for (int i = 0; i < n; ++i)
            {
                const float u = (float) i / (float) n, pk = peaks[(size_t) (reversed ? n - 1 - i : i)];
                const bool in = u >= trimA && u <= trimB;
                const float fade = jmin (1.0f, (u - trimA) / jmax (0.001f, fadeIn), (trimB - u) / jmax (0.001f, fadeOut));
                const float h = pk * w.getHeight() * 0.46f * (in ? jlimit (0.0f, 1.0f, fade) : 1.0f);
                g.setColour ((in ? t.accent : Colours::white.withAlpha (0.15f)).withAlpha (in ? 0.9f : 0.25f));
                g.fillRect (w.getX() + u * w.getWidth(), w.getCentreY() - h, jmax (1.0f, w.getWidth() / (float) n - 0.5f), h * 2.0f);
            }
            for (float u : { trimA, trimB })
            {
                const float x = w.getX() + u * w.getWidth();
                g.setColour (Colours::white); g.fillRect (x - 1.0f, w.getY(), 2.0f, w.getHeight());
                g.fillRoundedRectangle (x - 5.0f, w.getBottom() - 10.0f, 10.0f, 10.0f, 2.0f);
            }
            // fade corners
            g.setColour (Colours::white.withAlpha (0.7f));
            g.drawLine (w.getX() + trimA * w.getWidth(), w.getBottom(), w.getX() + (trimA + fadeIn) * w.getWidth(), w.getY(), 1.0f);
            g.drawLine (w.getX() + trimB * w.getWidth(), w.getBottom(), w.getX() + (trimB - fadeOut) * w.getWidth(), w.getY(), 1.0f);
        }
        else { g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.1f)); g.drawText (proc.sampleActive() || proc.chopActive() ? "a sample plays - its tools are in SAMPLER" : "...", w.toNearestInt(), Justification::centred); }
        if (noteT > 0) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (11.0f, true, 0.05f)); g.drawText (note, w.reduced (8, 2).toNearestInt(), Justification::topRight); }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 5);
        dragWav.setBounds (r.removeFromRight (128)); r.removeFromRight (6);
        saveBtn.setBounds (r.removeFromRight (64)); r.removeFromRight (4);
        revBtn.setBounds (r.removeFromRight (52)); r.removeFromRight (4);
        loopBtn.setBounds (r.removeFromRight (60)); r.removeFromRight (4);
        playBtn.setBounds (r.removeFromRight (60));
    }
    void mouseDown (const MouseEvent& e) override
    {
        const auto w = waveArea(); dragging = -1;
        if (! w.expanded (6).contains (e.position) || peaks.empty()) return;
        const float u = (e.position.x - w.getX()) / w.getWidth();
        const bool top = e.position.y < w.getY() + w.getHeight() * 0.3f;
        dragging = top ? (std::abs (u - (trimA + fadeIn)) < std::abs (u - (trimB - fadeOut)) ? 2 : 3) : (std::abs (u - trimA) < std::abs (u - trimB) ? 0 : 1);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragging < 0) return;
        const auto w = waveArea();
        const float u = jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth());
        if (dragging == 0) trimA = jmin (u, trimB - 0.05f);
        else if (dragging == 1) trimB = jmax (u, trimA + 0.05f);
        else if (dragging == 2) fadeIn = jlimit (0.0f, (trimB - trimA) * 0.5f, u - trimA);
        else fadeOut = jlimit (0.0f, (trimB - trimA) * 0.5f, trimB - u);
        repaint();
    }
    void mouseUp (const MouseEvent&) override { dragging = -1; }
    void mouseDoubleClick (const MouseEvent&) override { trimA = 0; trimB = 1; fadeIn = 0; fadeOut = 0.15f; reversed = false; revBtn.selected = false; revBtn.repaint(); repaint(); }
private:
    Rectangle<float> nameArea() const { return { 10, 4, 190, (float) getHeight() - 8 }; }
    Rectangle<float> waveArea() const { return { 206, 6, (float) playBtn.getX() - 216, (float) getHeight() - 12 }; }
    // the sound with your edits, as a WAV-ready sound
    kk::PairPtr edited()
    {
        if (audio.getNumSamples() == 0) render();
        if (audio.getNumSamples() == 0) return nullptr;
        const int n = audio.getNumSamples(), a = (int) (trimA * (float) n), b = jmax (a + 64, (int) (trimB * (float) n));
        AudioBuffer<float> out (audio.getNumChannels(), b - a);
        for (int c = 0; c < out.getNumChannels(); ++c) out.copyFrom (c, 0, audio, c, a, b - a);
        if (reversed) out.reverse (0, out.getNumSamples());
        const int fi = (int) (fadeIn / jmax (0.001f, trimB - trimA) * (float) out.getNumSamples()), fo = (int) (fadeOut / jmax (0.001f, trimB - trimA) * (float) out.getNumSamples());
        if (fi > 1) out.applyGainRamp (0, fi, 0.0f, 1.0f);
        if (fo > 1) out.applyGainRamp (out.getNumSamples() - fo, fo, 1.0f, 0.0f);
        return kk::PairLab::fromBuffer (out, rate, rate, name);
    }
    File writeTemp (const kk::PairSound& s)
    {
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("EVOLVE Dock"); dir.createDirectory();
        auto f = dir.getChildFile (File::createLegalFileName (name.isEmpty() ? String ("EVOLVE sound") : name) + ".wav");
        f.deleteFile();
        WavAudioFormat wav;
        if (auto os = std::unique_ptr<OutputStream> (f.createOutputStream()))
            if (auto w = std::unique_ptr<AudioFormatWriter> (wav.createWriterFor (os.get(), rate, (unsigned) s.audio.getNumChannels(), 24, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (s.audio, 0, s.audio.getNumSamples()); }
        return f;
    }
    void render()
    {
        audio.setSize (0, 0); peaks.clear();
        if (proc.sampleActive() || proc.chopActive()) return;
        auto g = proc.currentGenome(); if (! g.valid()) return;
        rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
        audio = proc.renderGenomeAudio (g, rate, 2.5);
        // the tail of silence is cut so the waveform fills the dock
        int last = audio.getNumSamples() - 1;
        while (last > 2000 && audio.getMagnitude (last - 256, 256) < 0.0015f) last -= 256;
        if (last + 1 < audio.getNumSamples()) { AudioBuffer<float> t (audio.getNumChannels(), last + 1); for (int c = 0; c < t.getNumChannels(); ++c) t.copyFrom (c, 0, audio, c, 0, last + 1); audio = std::move (t); }
        const int bins = 160; peaks.assign (bins, 0.0f); float mx = 1.0e-4f;
        for (int i = 0; i < bins; ++i) { const int a = audio.getNumSamples() * i / bins, b = audio.getNumSamples() * (i + 1) / bins; peaks[(size_t) i] = audio.getMagnitude (a, jmax (1, b - a)); mx = jmax (mx, peaks[(size_t) i]); }
        for (auto& p : peaks) p /= mx;
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        // follow the sound on the keys: re-draw its waveform a moment after it changed
        const String now = proc.currentName() + "|" + String (proc.labVersion()) + "|" + String ((int) proc.sampleActive()) + String ((int) proc.chopActive());
        if (now != lastKey) { lastKey = now; pending = 5; }
        if (pending > 0 && --pending == 0)
        {
            name = proc.sampleActive() || proc.chopActive() ? "SAMPLE: " + proc.currentName() : proc.currentName();
            trimA = 0; trimB = 1; fadeIn = 0; fadeOut = 0.15f;
            render(); repaint();
        }
        if (looping && ++loopT >= 30) { loopT = 0; proc.previewNote = 60; flash = 1.0f; }
        if (flash > 0) { flash = jmax (0.0f, flash - 0.12f); repaint(); }
        if (noteT > 0 && --noteT == 0) repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton playBtn { lnf }, loopBtn { lnf }, revBtn { lnf }, saveBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    AudioBuffer<float> audio;
    std::vector<float> peaks;
    String name, lastKey, note;
    double rate = 44100.0;
    float trimA = 0, trimB = 1, fadeIn = 0, fadeOut = 0.15f, flash = 0;
    bool looping = false, reversed = false;
    int dragging = -1, pending = 0, loopT = 0, noteT = 0;
};
