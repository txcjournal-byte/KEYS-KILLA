// v0.45 WORDS page (included by PluginEditor.cpp): an empty field - type a word, a sentence, a name, anything -> 10 sounds.
// Offline: meaning words (English + Czech) shape the recipe, every other word is hashed (the same word = the same 10 sounds,
// always - send a friend a word = a sound pack).  The words that shaped the sound light up under the field.
// Click a card = it is on your keys.  KEEP / PLANT / SAVE / DRAG WAV.
#include "Words.h"

class WordsPage : public Component, private Timer
{
public:
    WordsPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        const auto& t = kk::theme();
        field.setFont (kk::modern::font (40.0f, true, 0.02f));
        field.setJustification (Justification::centredLeft);
        field.setIndents (26, 14);
        field.setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
        field.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
        field.setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        field.setColour (TextEditor::textColourId, t.text);
        field.setColour (TextEditor::highlightColourId, t.accent.withAlpha (0.3f));
        field.setColour (CaretComponent::caretColourId, t.accent);
        field.setTextToShowWhenEmpty ("type anything ...", t.dim.withAlpha (0.55f));
        field.setInputRestrictions (80);
        field.onTextChange = [this] { typedAt = Time::getMillisecondCounter(); dirty = true; };
        field.onReturnKey = [this] { make(); if (! res.sounds.empty()) choose (0); };
        addAndMakeVisible (field);
        auto btn = [this] (HotButton& b, const String& tx, const String& tip, std::function<void()> fn) { b.setButtonText (tx); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (keepBtn, "KEEP", "Keep the chosen sound on the shelf below", [this]
        {
            if (! chosen.valid()) return;
            for (auto& k : kept) if (k.name == chosen.name) return;
            kept.insert (kept.begin(), chosen); if (kept.size() > 12) kept.pop_back();
            note = "kept: " + chosen.name; repaint();
        });
        btn (plantBtn, "PLANT IN EVOLVE", "The chosen sound becomes the seed of a new EVOLVE tree", [this] { if (chosen.valid()) { proc.evoSeedGenome (chosen); note = "planted - open EVOLVE"; repaint(); } });
        btn (saveBtn, "SAVE", "Save the chosen sound into MY SOUNDS (pick a folder / sound kit)", [this]
        {
            if (! chosen.valid()) return;
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (chosen, rate, 3.0), rate, rate, chosen.name);
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<WordsPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
        });
        dragWav.makeFile = [this] { return chosen.valid() ? proc.exportGenomeWav (chosen) : File(); };
        dragWav.setTooltip ("Drag the chosen sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        startTimerHz (30);
    }
    ~WordsPage() override { stopTimer(); }

    void debugType (const String& text)
    {
        field.setText (text, false);
        make(); born = 1.0f;
        if (cards.size() > 7) { kept = { cards[3].g, cards[7].g }; select (0, false); keptSel = -1; }
        note = "kept: " + (kept.empty() ? String() : kept.front().name);
        repaint();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("WORDS", 24, 12, 160, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("type a word, a sentence, a name, anything - 10 sounds appear.  every word in the world has its sounds (the same word = the same sounds, always)",
                    160, 18, getWidth() - 184, 28, Justification::centredLeft);
        drawField (g);
        drawWords (g);
        if (res.sounds.empty()) drawEmpty (g);
        else for (int i = 0; i < (int) cards.size(); ++i) drawCard (g, i);
        drawShelf (g);
    }
    void resized() override
    {
        field.setBounds (fieldArea().reduced (8, 6).withTrimmedRight (60));
        const auto b = barArea();
        int x = b.getRight() - 14;
        auto place = [&] (Component& c, int w, int h) { x -= w; c.setBounds (x, b.getCentreY() - h / 2, w, h); x -= 10; };
        place (dragWav, 160, 46); place (saveBtn, 110, 42); place (plantBtn, 190, 42); place (keepBtn, 110, 42);
    }
    void mouseMove (const MouseEvent& e) override { const int h = cardAt (e.position); if (h != hover) { hover = h; repaint(); } }
    void mouseExit (const MouseEvent&) override { if (hover >= 0) { hover = -1; repaint(); } }
    void mouseDown (const MouseEvent& e) override
    {
        if (const int i = cardAt (e.position); i >= 0) { choose (i); return; }
        for (int i = 0; i < (int) kept.size(); ++i)
            if (shelfSlot (i).contains (e.position)) { chosen = kept[(size_t) i]; sel = -1; keptSel = i; pulse = 1.0f; proc.alcUse (chosen, true); note = chosen.name + " is on your keys"; repaint(); return; }
        if (fieldArea().contains (e.getPosition())) field.grabKeyboardFocus();
    }
private:
    struct Card { kk::words::Recipe r; KeysKillaProcessor::Genome g; };
    Rectangle<int> fieldArea() const { return { 24, 62, getWidth() - 48, 78 }; }
    Rectangle<int> wordsArea() const { return { 24, 148, getWidth() - 48, 34 }; }
    Rectangle<int> cardsArea() const { return { 24, 192, getWidth() - 48, getHeight() - 192 - 138 }; }
    Rectangle<int> barArea() const { return { 24, getHeight() - 124, getWidth() - 48, 108 }; }
    Rectangle<float> cardRect (int i) const
    {
        const auto a = cardsArea().toFloat();
        const float gap = 14, w = (a.getWidth() - 4 * gap) / 5, h = (a.getHeight() - gap) / 2;
        return { a.getX() + (float) (i % 5) * (w + gap), a.getY() + (float) (i / 5) * (h + gap), w, h };
    }
    Rectangle<float> shelfSlot (int i) const { const auto a = barArea().toFloat(); return { a.getX() + 16 + (float) i * 64.0f, a.getY() + 44, 54, 54 }; }
    int cardAt (Point<float> p) const { for (int i = 0; i < (int) cards.size(); ++i) if (cardRect (i).contains (p)) return i; return -1; }

    void make()
    {
        dirty = false;
        const auto text = field.getText();
        if (text == lastText) return;
        lastText = text;
        res = kk::words::type (text, 10);
        cards.clear();
        for (auto& r : res.sounds)
        {
            Card c; c.r = r;
            c.g = proc.sculpt (proc.alchemy (r.exc, r.body, r.matter, r.size, r.seed), r.stretch, r.bright, r.heat, r.cool, r.split);
            if (c.g.valid()) c.g.name = r.name;
            cards.push_back (c);
        }
        sel = -1; born = 0.0f;
        repaint();
    }
    void select (int i, bool hear)
    {
        if (! isPositiveAndBelow (i, (int) cards.size()) || ! cards[(size_t) i].g.valid()) return;
        sel = i; keptSel = -1; chosen = cards[(size_t) i].g; pulse = 1.0f;
        proc.alcUse (chosen, hear);
        note = chosen.name + " is on your keys";
        repaint();
    }
    void choose (int i) { select (i, true); }

    // ---- drawing
    void drawField (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto r = fieldArea().toFloat();
        const bool focus = field.hasKeyboardFocus (true);
        kk::modern::well (g, r, 22.0f);
        const float glow = 0.35f + 0.25f * std::sin (phase * 1.3f);
        g.setGradientFill (ColourGradient (t.accent.withAlpha ((focus ? 0.22f : 0.12f) * glow), r.getX(), r.getCentreY(), Colour (0xff8f7cff).withAlpha (0.16f * glow), r.getRight(), r.getCentreY(), false));
        g.fillRoundedRectangle (r, 22.0f);
        g.setColour (t.accent.withAlpha (focus ? 0.9f : 0.45f)); g.drawRoundedRectangle (r.reduced (0.8f), 22.0f, focus ? 2.0f : 1.3f);
        // a quill on the right: the words become sounds
        const auto q = Rectangle<float> (r.getRight() - 62, r.getY() + 14, 44, r.getHeight() - 28);
        Path p; p.startNewSubPath (q.getX() + 6, q.getBottom() - 4);
        for (int k = 0; k <= 24; ++k) { const float x = q.getX() + 6 + (float) k / 24.0f * (q.getWidth() - 12); p.lineTo (x, q.getCentreY() + std::sin ((float) k * 0.9f + phase * 3.0f) * q.getHeight() * 0.32f * std::sin ((float) k / 24.0f * MathConstants<float>::pi)); }
        g.setColour (t.accent.withAlpha (0.8f)); g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    void drawWords (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = wordsArea().toFloat();
        if (res.words.empty())
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, true, 0.06f));
            g.drawText (String ("try:   dark metal rain    ") + String::fromUTF8 ("déšť    vesmír") + "    your name    a sentence    nonsense like  zorblax", a.toNearestInt(), Justification::centredLeft);
            return;
        }
        float x = a.getX();
        for (auto& w : res.words)
        {
            const String tx = w.stop ? w.text : w.text.toUpperCase();
            const auto f = kk::modern::font (w.stop ? 12.0f : 14.0f, true, 0.12f);
            GlyphArrangement ga; ga.addLineOfText (f, tx, 0, 0);
            const float tw = ga.getBoundingBox (0, -1, true).getWidth() + (w.stop ? 10.0f : 34.0f);
            if (x + tw > a.getRight() - 300) break;
            const auto pill = Rectangle<float> (x, a.getY() + 3, tw, a.getHeight() - 6);
            if (w.known)
            {
                const float lit = 0.75f + 0.25f * std::sin (phase * 2.0f + x * 0.01f);
                g.setColour (t.accent.withAlpha (0.18f * lit)); g.fillRoundedRectangle (pill.expanded (3), 16);
                g.setGradientFill (ColourGradient (t.accent.withAlpha (0.55f * lit), pill.getX(), pill.getY(), Colour (0xffff4fd8).withAlpha (0.35f * lit), pill.getRight(), pill.getBottom(), false));
                g.fillRoundedRectangle (pill, 14);
                g.setColour (Colours::white); g.setFont (f); g.drawText (tx, pill.toNearestInt(), Justification::centred);
            }
            else if (! w.stop)
            {
                g.setColour (t.text.withAlpha (0.08f)); g.fillRoundedRectangle (pill, 14);
                g.setColour (t.text.withAlpha (0.45f)); g.drawRoundedRectangle (pill.reduced (0.5f), 14, 1.0f);
                g.setColour (t.text.withAlpha (0.85f)); g.setFont (f); g.drawText ("# " + tx, pill.toNearestInt(), Justification::centred);
            }
            else { g.setColour (t.dim.withAlpha (0.6f)); g.setFont (f); g.drawText (tx, pill.toNearestInt(), Justification::centred); }
            x += tw + 8;
        }
        int known = 0, hashed = 0; for (auto& w : res.words) { known += w.known; hashed += ! w.known && ! w.stop; }
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawText ((known > 0 ? String (known) + " meaning " + (known == 1 ? "word" : "words") + " lit" : String ("no meaning words")) + (hashed > 0 ? "   ·   # " + String (hashed) + " hashed" : String()),
                    (int) a.getRight() - 290, (int) a.getY(), 290, (int) a.getHeight(), Justification::centredRight);
    }
    void drawEmpty (Graphics& g)
    {
        const auto& t = kk::theme();
        for (int i = 0; i < 10; ++i)
        {
            const auto r = cardRect (i);
            const float a = 0.08f + 0.05f * std::sin (phase * 1.5f - (float) i * 0.5f);
            g.setColour (t.text.withAlpha (a)); g.drawRoundedRectangle (r.reduced (1), 20, 1.4f);
            g.setColour (t.text.withAlpha (a * 0.5f)); g.fillEllipse (r.withSizeKeepingCentre (40, 40).translated (0, -14));
        }
        const auto c = cardsArea().toFloat();
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (26.0f, true, 0.06f));
        g.drawText ("every word has its sounds", c.withSizeKeepingCentre (c.getWidth(), 40).translated (0, -20).toNearestInt(), Justification::centred);
        g.setColour (t.dim); g.setFont (kk::modern::font (14.0f, true, 0.04f));
        g.drawText ("dark, glass, rain, metal, soft, angry, space, water, fire ... in English or Czech.  names and nonsense work too", c.withSizeKeepingCentre (c.getWidth(), 24).translated (0, 18).toNearestInt(), Justification::centred);
    }
    void drawCard (Graphics& g, int i)
    {
        const auto& t = kk::theme();
        const auto& cd = cards[(size_t) i];
        if (! cd.g.valid()) return;
        const float appear = jlimit (0.0f, 1.0f, born * 3.0f - (float) i * 0.18f);
        if (appear <= 0) return;
        auto r = cardRect (i).translated (0, (1.0f - appear) * 24.0f);
        const bool on = i == sel, hov = i == hover;
        const auto ec = Colour (kk::alc::exciters()[(size_t) cd.r.exc].colour), bc = Colour (kk::alc::bodies()[(size_t) cd.r.body].colour);
        const float al = appear;
        g.setColour (Colours::black.withAlpha (0.3f * al)); g.fillRoundedRectangle (r.translated (0, 6), 20);
        g.setGradientFill (ColourGradient (Colour (0xff14111f).withAlpha (al), r.getX(), r.getY(), Colour (0xff07080e).withAlpha (al), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 20);
        g.setGradientFill (ColourGradient (ec.withAlpha ((on ? 0.42f : hov ? 0.32f : 0.24f) * al), r.getX(), r.getY(), bc.withAlpha (0.22f * al), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 20);
        if (on) { g.setColour (Colours::white.withAlpha (0.25f * pulse)); g.drawRoundedRectangle (r.expanded (4 + 6 * (1 - pulse)), 24, 3.0f); }
        g.setColour (on ? Colours::white : Colours::white.withAlpha (hov ? 0.4f : 0.18f)); g.drawRoundedRectangle (r.reduced (0.5f), 20, on ? 2.2f : 1.2f);
        // the matter of this sound: crystal when glassy, a soft blob when muddy, bigger when giant, frost / heat / a torn twin
        const auto c = Point<float> (r.getCentreX(), r.getY() + r.getHeight() * 0.38f);
        const float rad = jmin (r.getWidth(), r.getHeight()) * (0.15f + 0.1f * cd.r.size) * (on ? 1.0f + 0.06f * pulse : 1.0f);
        auto blob = [&] (Point<float> cc, float rr, float seedPh)
        {
            Path p; const int n = 56;
            for (int k = 0; k <= n; ++k)
            {
                const float a = (float) k / (float) n * MathConstants<float>::twoPi;
                const float crystal = std::abs (std::fmod (a / MathConstants<float>::twoPi * 6.0f, 1.0f) - 0.5f) * 2.0f;
                const float soft = 0.07f * std::sin (a * 3.0f + phase * 2.0f + seedPh) + 0.04f * std::sin (a * 5.0f - phase * 3.0f);
                float kk2 = (1.0f - cd.r.matter) * (0.7f + 0.3f * crystal) + cd.r.matter * (1.0f + soft);
                kk2 += cd.r.heat * 0.06f * std::sin (a * 19.0f + phase * 8.0f);
                const auto pt = cc + Point<float> (std::cos (a) * (1.0f + 0.25f * cd.r.stretch), std::sin (a)) * rr * kk2;
                if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            g.setColour (ec.withAlpha (0.22f * al)); g.fillPath (p, AffineTransform::scale (1.4f, 1.4f, cc.x, cc.y));
            const auto top = ec.brighter (0.3f + 0.4f * jmax (0.0f, cd.r.bright)), bot = bc.darker (0.3f + 0.5f * jmax (0.0f, -cd.r.bright));
            g.setGradientFill (ColourGradient (top.withAlpha (al), cc.x - rr, cc.y - rr, bot.withAlpha (al), cc.x + rr, cc.y + rr, false));
            g.fillPath (p);
            g.setColour (Colours::white.withAlpha ((0.25f + 0.5f * cd.r.cool) * al)); g.strokePath (p, PathStrokeType (1.2f + cd.r.cool));
        };
        if (cd.r.split > 0.25f) { blob (c - Point<float> (rad * 0.55f, 0), rad * 0.8f, (float) i); blob (c + Point<float> (rad * 0.6f, rad * 0.1f), rad * 0.6f, (float) i + 2.0f); }
        else blob (c, rad, (float) i);
        // number
        g.setColour (Colours::white.withAlpha (0.5f * al)); g.setFont (kk::modern::font (12.0f, true, 0.2f));
        g.drawText (String (i + 1).paddedLeft ('0', 2), r.reduced (14, 10).toNearestInt(), Justification::topLeft);
        // name + what it is made of
        g.setColour (Colours::white.withAlpha (al)); g.setFont (kk::modern::font (16.5f, true, 0.04f));
        g.drawFittedText (cd.g.name, r.withTrimmedTop (r.getHeight() * 0.66f).withHeight (24).reduced (12, 0).toNearestInt(), Justification::centred, 1, 0.7f);
        g.setColour (Colours::white.withAlpha (0.62f * al)); g.setFont (kk::modern::font (10.5f, true, 0.1f));
        g.drawFittedText (String (kk::alc::exciters()[(size_t) cd.r.exc].word).toUpperCase() + " x " + String (kk::alc::bodies()[(size_t) cd.r.body].word).toUpperCase() + "  ·  " + kk::alc::matterWord (cd.r.matter).toUpperCase(),
                          r.withTrimmedTop (r.getHeight() * 0.66f + 26).withHeight (16).reduced (10, 0).toNearestInt(), Justification::centred, 1, 0.7f);
        // tags: what the words did
        StringArray tags;
        if (cd.r.bright < -0.3f) tags.add ("DARK"); else if (cd.r.bright > 0.3f) tags.add ("BRIGHT");
        if (cd.r.stretch < -0.3f) tags.add ("SHORT"); else if (cd.r.stretch > 0.3f) tags.add ("LONG");
        if (cd.r.heat > 0.4f) tags.add ("HOT"); if (cd.r.cool > 0.4f) tags.add ("FROZEN"); if (cd.r.split > 0.25f) tags.add ("TORN");
        if (cd.r.size > 0.75f) tags.add ("GIANT"); else if (cd.r.size < 0.25f) tags.add ("TINY");
        g.setFont (kk::modern::font (9.5f, true, 0.15f));
        float tx = r.getCentreX() - (float) jmin (3, tags.size()) * 31.0f;
        for (int k = 0; k < jmin (3, tags.size()); ++k)
        {
            const auto tr = Rectangle<float> (tx + (float) k * 62.0f, r.getBottom() - 30, 58, 18);
            g.setColour (Colours::white.withAlpha (0.12f * al)); g.fillRoundedRectangle (tr, 9);
            g.setColour (Colours::white.withAlpha (0.8f * al)); g.drawText (tags[k], tr.toNearestInt(), Justification::centred);
        }
        ignoreUnused (t);
    }
    void drawShelf (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = barArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("KEPT", (int) a.getX() + 16, (int) a.getY() + 12, 60, 20, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawText (kept.empty() ? String ("click a card = it is on your keys.  KEEP puts it here") : String ("click = on your keys again"), (int) a.getX() + 72, (int) a.getY() + 13, 420, 18, Justification::centredLeft);
        for (int i = 0; i < (int) kept.size(); ++i)
        {
            const auto s = shelfSlot (i);
            if (s.getRight() > a.getRight() - 640) break;
            const auto col = Colour::fromHSV ((float) (kk::words::hashString (kept[(size_t) i].name) % 360u) / 360.0f, 0.55f, 0.95f, 1.0f);
            g.setGradientFill (ColourGradient (col, s.getX(), s.getY(), col.darker (0.8f), s.getRight(), s.getBottom(), false));
            g.fillEllipse (s.reduced (6));
            if (i == keptSel) { g.setColour (Colours::white); g.drawEllipse (s.reduced (2), 2.2f); }
        }
        if (note.isNotEmpty())
        {
            g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.03f));
            g.drawFittedText (note, Rectangle<int> ((int) a.getRight() - 640 - 10, (int) a.getY() + 8, 620, 18).withX (keepBtn.getX()).withWidth (dragWav.getRight() - keepBtn.getX()), Justification::centredRight, 1, 0.8f);
        }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.04f;
        if (born < 1.0f) born = jmin (1.0f, born + 0.04f);
        if (pulse > 0) pulse = jmax (0.0f, pulse - 0.04f);
        if (dirty && Time::getMillisecondCounter() - typedAt > 280) make();   // a short pause in typing = the sounds appear
        repaint();
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    TextEditor field;
    HotButton keepBtn { lnf }, plantBtn { lnf }, saveBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    kk::words::Result res;
    std::vector<Card> cards;
    std::vector<KeysKillaProcessor::Genome> kept;
    KeysKillaProcessor::Genome chosen;
    String note, lastText;
    int sel = -1, hover = -1, keptSel = -1;
    uint32 typedAt = 0;
    bool dirty = false;
    float phase = 0, born = 1.0f, pulse = 0;
};
