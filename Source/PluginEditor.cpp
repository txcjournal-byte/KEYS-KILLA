#include "PluginEditor.h"

using namespace lnui;

namespace
{
const juce::String dot = juce::String::fromUTF8 (" \xc2\xb7 ");

juce::String formatBpm (double bpm) { return juce::String (bpm, 1); }

// ---------------------------------------------------------------------------------------------------------------
// small vector icons (no glyph fonts needed)
enum class Icon { Play, Stop, Reroll, Up, Down };

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour c)
{
    const float s = juce::jmin (r.getWidth(), r.getHeight());
    const auto ctr = r.getCentre();
    juce::Path p;
    switch (icon)
    {
        case Icon::Play:
            p.addTriangle (ctr.x - s * 0.22f, ctr.y - s * 0.28f, ctr.x - s * 0.22f, ctr.y + s * 0.28f, ctr.x + s * 0.30f, ctr.y);
            g.setColour (c); g.fillPath (p);
            break;
        case Icon::Stop:
            g.setColour (c); g.fillRect (juce::Rectangle<float> (s * 0.46f, s * 0.46f).withCentre (ctr));
            break;
        case Icon::Reroll:
        {
            const float rad = s * 0.27f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.55f, juce::MathConstants<float>::twoPi - 0.1f, true);
            g.setColour (c);
            g.strokePath (p, juce::PathStrokeType (juce::jmax (1.5f, s * 0.085f), juce::PathStrokeType::mitered, juce::PathStrokeType::square));
            juce::Path head;
            const float ax = ctr.x + rad * std::sin (0.55f), ay = ctr.y - rad * std::cos (0.55f);
            head.addTriangle (ax - s * 0.02f, ay - s * 0.16f, ax + s * 0.17f, ay + s * 0.02f, ax - s * 0.12f, ay + s * 0.12f);
            g.fillPath (head);
            break;
        }
        case Icon::Up:
            p.addTriangle (ctr.x - s * 0.28f, ctr.y + s * 0.16f, ctr.x + s * 0.28f, ctr.y + s * 0.16f, ctr.x, ctr.y - s * 0.2f);
            g.setColour (c); g.fillPath (p);
            break;
        case Icon::Down:
            p.addTriangle (ctr.x - s * 0.28f, ctr.y - s * 0.16f, ctr.x + s * 0.28f, ctr.y - s * 0.16f, ctr.x, ctr.y + s * 0.2f);
            g.setColour (c); g.fillPath (p);
            break;
    }
}

class IconButton : public juce::Button
{
public:
    IconButton (const juce::String& name, Icon i) : juce::Button (name), icon (i) {}
    void setIcon (Icon i) { if (i != icon) { icon = i; repaint(); } }
    void setLit (bool l) { if (l != lit) { lit = l; repaint(); } }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (col::bg);
        g.fillRect (r);
        if (lit) { g.setColour (col::green.withAlpha (0.16f)); g.fillRect (r); }
        else if (over) { g.setColour (col::green.withAlpha (0.07f)); g.fillRect (r); }
        if (down) { g.setColour (col::green.withAlpha (0.12f)); g.fillRect (r); }
        g.setColour (lit ? col::green : (over ? col::dim : col::border.brighter (0.3f)));
        g.drawRect (r, 1.0f);
        const auto c = lit ? col::green : (over ? col::text : col::dim.brighter (0.2f));
        if (lit) drawIcon (g, icon, r.expanded (1.0f), c.withAlpha (0.25f));
        drawIcon (g, icon, r, c);
    }

private:
    Icon icon;
    bool lit = false;
};

// GENERATE: the big TrapWin-red button (two text lines + pixel clover)
class GenerateButton : public juce::Button
{
public:
    explicit GenerateButton (juce::LookAndFeel& l) : juce::Button ("GENERATE"), lf (l) { getProperties().set ("ln-style", "red"); }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        lf.drawButtonBackground (g, *this, col::red, over, down);
        auto r = getLocalBounds().toFloat().reduced (10).translated (0, down ? 1.0f : 0.0f);
        const auto ink = juce::Colour (0xfffff2f2);
        drawPixelClover (g, r.removeFromTop (r.getHeight() * 0.42f).reduced (0, 2), col::green, true);
        drawGlowText (g, "GENERATE", r.removeFromTop (r.getHeight() * 0.56f), juce::Justification::centred, ink, pixelFont (15.0f), 0.5f);
        g.setColour (ink.withAlpha (0.8f));
        g.setFont (monoFont (11.0f, true));
        g.drawText ("12 NEW MELODIES", r, juce::Justification::centredTop, false);
    }

private:
    juce::LookAndFeel& lf;
};

void styleSeg (juce::TextButton& b, const juce::String& tip = {})
{
    b.getProperties().set ("ln-style", "seg");
    b.setClickingTogglesState (false);
    if (tip.isNotEmpty()) b.setTooltip (tip);
}
} // namespace

// ===============================================================================================================
class TempoLcd : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TempoLcd (LuckyNotesProcessor& p) : proc (p)
    {
        setTooltip ("Tempo of the MIDI files and the preview. SYNC TO PROJECT follows your DAW. "
                    "Sync off: drag up/down or scroll to set it (hold Shift for 0.1 steps).");
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    void update()
    {
        const auto s = proc.getSettings();
        const double bpm = proc.getEffectiveBpm();
        const bool host = proc.hasHostTempo();
        const bool sync = s.sync;
        if (bpm != shownBpm || host != shownHost || sync != shownSync)
        {
            shownBpm = bpm; shownHost = host; shownSync = sync;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawLcd (g, r);
        const bool following = shownSync && shownHost;
        juce::String mode = following ? "HOST SYNC" : (shownSync ? "NO HOST - MANUAL" : "MANUAL");
        g.setFont (pixelFont (7.0f));
        g.setColour (col::amber.withAlpha (0.55f));
        g.drawText (mode, r.reduced (8, 6).withHeight (9), juce::Justification::topLeft, false);
        if (! following)
        {
            // small up/down arrows: "drag me"
            juce::Path p;
            const float x = r.getRight() - 12.0f, y = r.getY() + 6.0f;
            p.addTriangle (x - 3.5f, y + 3.5f, x + 3.5f, y + 3.5f, x, y);
            p.addTriangle (x - 3.5f, y + 5.5f, x + 3.5f, y + 5.5f, x, y + 9.0f);
            g.fillPath (p);
        }
        auto body = r.reduced (8, 4).withTrimmedTop (10);
        auto unit = body.removeFromRight (34);
        drawGlowText (g, formatBpm (shownBpm), body, juce::Justification::centredRight, col::amber, lcdFont (58.0f), 1.0f);
        drawGlowText (g, "BPM", unit.withTrimmedTop (20), juce::Justification::centred, col::amber.withAlpha (0.7f), lcdFont (22.0f), 0.0f);
    }

    void mouseDown (const juce::MouseEvent&) override { dragStartBpm = proc.getSettings().manualBpm; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! editable()) return;
        const double perPixel = e.mods.isShiftDown() ? 0.02 : 0.25;
        proc.setManualBpm (dragStartBpm - e.getDistanceFromDragStartY() * perPixel);
        update();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! editable()) return;
        const double step = e.mods.isShiftDown() ? 0.1 : 1.0;
        proc.setManualBpm (proc.getSettings().manualBpm + (w.deltaY > 0 ? step : -step) * (w.isReversed ? -1 : 1));
        update();
    }

private:
    bool editable() const { return ! (proc.getSettings().sync && proc.hasHostTempo()); }

    LuckyNotesProcessor& proc;
    double shownBpm = -1, dragStartBpm = 140;
    bool shownHost = false, shownSync = true;
};

// ===============================================================================================================
class KeyLcd : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit KeyLcd (LuckyNotesProcessor& p) : proc (p)
    {
        setTooltip ("Key of the melodies. Drag up/down or scroll to change the root. LUCKY picks a random key on every GENERATE.");
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    void update()
    {
        const auto s = proc.getSettings();
        const auto set = proc.getSet();
        const ln::Key k = set != nullptr ? set->settings.key : s.resolvedKey;
        const juce::String t (ln::keyDisplayName (k));
        if (t != text || s.lucky != lucky) { text = t; lucky = s.lucky; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawLcd (g, r);
        g.setFont (pixelFont (7.0f));
        g.setColour (col::amber.withAlpha (0.55f));
        g.drawText (lucky ? "LUCKY KEY" : "ROOT / SCALE", r.reduced (8, 6).withHeight (9), juce::Justification::topLeft, false);
        drawGlowText (g, text, r.reduced (8, 4).withTrimmedTop (10), juce::Justification::centred, col::amber, lcdFont (50.0f), 1.0f);
    }

    void mouseDown (const juce::MouseEvent&) override { startRoot = proc.getSettings().root; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int steps = -e.getDistanceFromDragStartY() / 14;
        const auto s = proc.getSettings();
        const int root = ln::floorMod (startRoot + steps, 12);
        if (root != s.root || s.lucky) proc.setKey (root, s.minor);
        update();
    }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const auto s = proc.getSettings();
        proc.setKey (ln::floorMod (s.root + ((w.deltaY > 0) != w.isReversed ? 1 : -1), 12), s.minor);
        update();
    }

private:
    LuckyNotesProcessor& proc;
    juce::String text;
    bool lucky = false;
    int startRoot = 0;
};

// ===============================================================================================================
class PianoRoll : public juce::Component
{
public:
    void setMelody (const ln::Melody* m)
    {
        if (m == nullptr) { notes.clear(); repaint(); return; }
        notes = m->notes;
        length = m->lengthBeats;
        lo = 127; hi = 0;
        for (auto& n : notes) { lo = juce::jmin (lo, n.pitch); hi = juce::jmax (hi, n.pitch); }
        if (notes.empty()) { lo = 60; hi = 72; }
        repaint();
    }
    void setPlayhead (double beat) // < 0 = not playing
    {
        if (std::abs (beat - playhead) > 1e-4) { playhead = beat; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::bg);
        g.fillRect (r);
        const bool playing = playhead >= 0;
        const float w = r.getWidth(), h = r.getHeight();
        const int bars = (int) std::round (length / 4.0);
        for (int b = 0; b <= bars * 4; ++b)
        {
            const float x = r.getX() + (float) (b / (bars * 4.0)) * w;
            g.setColour (b % 4 == 0 ? col::border.brighter (0.15f) : col::border.withAlpha (0.35f));
            g.fillRect (x, r.getY(), 1.0f, h);
        }
        const int range = juce::jmax (8, hi - lo + 1);
        const float rowH = (h - 4.0f) / (float) range;
        const int mid = (lo + hi) / 2;
        const int base = mid - range / 2;
        for (int pass = 0; pass < 2; ++pass)
            for (auto& n : notes)
            {
                const float x = r.getX() + (float) (n.start / length) * w;
                const float nw = juce::jmax (2.0f, (float) (n.length / length) * w - 1.0f);
                const float y = r.getBottom() - 2.0f - (float) (n.pitch - base + 1) * rowH;
                auto nr = juce::Rectangle<float> (x, y, nw, juce::jmax (2.0f, rowH)).getIntersection (r.reduced (0, 1));
                const bool past = playing && n.start <= playhead;
                if (pass == 0)
                {
                    if (playing) { g.setColour (col::green.withAlpha (0.18f)); g.fillRect (nr.expanded (1.5f)); }
                }
                else
                {
                    g.setColour (playing ? (past ? col::green : col::green.withAlpha (0.8f)) : col::green.withAlpha (0.72f));
                    g.fillRect (nr);
                }
            }
        if (playing)
        {
            const float x = r.getX() + (float) (playhead / length) * w;
            g.setColour (col::text.withAlpha (0.25f));
            g.fillRect (x - 2.0f, r.getY(), 5.0f, h);
            g.setColour (col::text);
            g.fillRect (x, r.getY(), 1.0f, h);
        }
        g.setColour (playing ? col::green.withAlpha (0.55f) : col::border);
        g.drawRect (r, 1.0f);
    }

private:
    std::vector<ln::Note> notes;
    double length = 32.0, playhead = -1.0;
    int lo = 60, hi = 72;
};

// ===============================================================================================================
class DragMidiButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragMidiButton (LuckyNotesProcessor& p, int r, std::function<void (const juce::String&)> flashFn)
        : proc (p), row (r), flash (std::move (flashFn))
    {
        setTooltip ("Drag this straight into your DAW (FL Studio playlist / piano roll, an Ableton track...)");
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (col::bg);
        g.fillRect (r);
        if (dragging) { g.setColour (col::green.withAlpha (0.22f)); g.fillRect (r); }
        else if (isMouseOver()) { g.setColour (col::green.withAlpha (0.09f)); g.fillRect (r); }
        g.setColour (col::green.withAlpha (isMouseOver() || dragging ? 0.95f : 0.55f));
        g.drawRect (r, 1.0f);
        // grip dots
        auto grip = r.removeFromLeft (22).reduced (7, 6);
        g.setColour (col::green.withAlpha (0.8f));
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 2; ++x)
                g.fillRect (grip.getX() + x * 5.0f, grip.getCentreY() - 6.0f + y * 5.0f, 2.0f, 2.0f);
        drawGlowText (g, "DRAG MIDI", r.withTrimmedRight (8), juce::Justification::centred, col::green, pixelFont (8.0f), 0.8f);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 5) return;
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("TrapWin Lucky Notes");
        const auto f = proc.writeMidiToFolder (row, dir);
        if (! f.existsAsFile()) { flash ("Could not write the MIDI file"); return; }
        dragging = true;
        repaint();
        juce::Component::SafePointer<DragMidiButton> safe (this);
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [safe]
        {
            if (safe != nullptr) { safe->dragging = false; safe->repaint(); }
        });
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! dragging && e.getDistanceFromDragStart() < 5)
            flash ("Hold and drag DRAG MIDI into your DAW - or use EXPORT ALL MIDI");
        dragging = false;
        repaint();
    }

private:
    LuckyNotesProcessor& proc;
    int row;
    std::function<void (const juce::String&)> flash;
    bool dragging = false;
};

// ===============================================================================================================
class MelodyRow : public juce::Component
{
public:
    MelodyRow (LuckyNotesProcessor& p, int r, std::function<void (const juce::String&)> flash)
        : proc (p), row (r), play ("play", Icon::Play), reroll ("reroll", Icon::Reroll), drag (p, r, std::move (flash))
    {
        play.setTooltip ("Preview (loops in time with your DAW when it plays)");
        reroll.setTooltip ("Re-roll just this melody");
        play.onClick = [this]
        {
            if (proc.getPreviewRow() == row) proc.stopPreview();
            else proc.startPreview (row);
        };
        reroll.onClick = [this] { proc.reroll (row); };
        addAndMakeVisible (play);
        addAndMakeVisible (roll);
        addAndMakeVisible (reroll);
        addAndMakeVisible (drag);
    }

    void setMelody (const ln::Melody* m)
    {
        roll.setMelody (m);
        name = m != nullptr ? juce::String (m->characterName) : juce::String();
        count = m != nullptr ? (int) m->notes.size() : 0;
        repaint();
    }

    void setPlaying (bool p, double beat)
    {
        if (p != playing) { playing = p; play.setIcon (p ? Icon::Stop : Icon::Play); play.setLit (p); repaint(); }
        roll.setPlayhead (p ? beat : -1.0);
    }

    void resized() override
    {
        // columns (row-local): number | play | character | piano roll | re-roll | drag
        const int h = getHeight();
        play.setBounds (40, (h - 22) / 2, 30, 22);
        roll.setBounds (204, 2, 546, h - 4);
        reroll.setBounds (758, (h - 22) / 2, 30, 22);
        drag.setBounds (796, (h - 24) / 2, 144, 24);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (playing) { g.setColour (col::green.withAlpha (0.05f)); g.fillRect (r); }
        const juce::String num = juce::String (row + 1).paddedLeft ('0', 2);
        drawGlowText (g, num, r.withX (4).withWidth (32), juce::Justification::centredLeft,
                      playing ? col::green : col::dim, lcdFont (26.0f), playing ? 1.0f : 0.0f);
        g.setFont (monoFont (12.5f, true));
        g.setColour (playing ? col::green : col::text);
        g.drawText (name, juce::Rectangle<float> (80, 0, 120, r.getHeight() - 9), juce::Justification::bottomLeft, false);
        g.setFont (monoFont (10.0f));
        g.setColour (col::dim);
        g.drawText (juce::String (count) + " NOTES", juce::Rectangle<float> (80, r.getHeight() * 0.5f + 1, 120, 12),
                    juce::Justification::centredLeft, false);
    }

private:
    LuckyNotesProcessor& proc;
    int row;
    IconButton play, reroll;
    PianoRoll roll;
    DragMidiButton drag;
    juce::String name;
    int count = 0;
    bool playing = false;
};

// ===============================================================================================================
class LuckyNotesContent : public juce::Component
{
public:
    LuckyNotesContent (LuckyNotesProcessor& p, juce::LookAndFeel& lf)
        : proc (p), tempo (p), key (p), generate (lf), up ("up", Icon::Up), down ("down", Icon::Down)
    {
        setOpaque (true);
        addAndMakeVisible (tempo);
        addAndMakeVisible (key);

        styleSeg (sync, "Follow the tempo of your DAW project");
        sync.onClick = [this] { proc.setSync (! proc.getSettings().sync); refresh(); };
        addAndMakeVisible (sync);

        up.setTooltip ("Root up");
        down.setTooltip ("Root down");
        up.onClick = [this] { auto s = proc.getSettings(); proc.setKey ((s.root + 1) % 12, s.minor); refresh(); };
        down.onClick = [this] { auto s = proc.getSettings(); proc.setKey ((s.root + 11) % 12, s.minor); refresh(); };
        addAndMakeVisible (up);
        addAndMakeVisible (down);

        styleSeg (minorBtn, "Minor scale");
        styleSeg (majorBtn, "Major scale");
        styleSeg (luckyBtn, "LUCKY: every GENERATE also picks a random key");
        minorBtn.onClick = [this] { proc.setKey (proc.getSettings().root, true); refresh(); };
        majorBtn.onClick = [this] { proc.setKey (proc.getSettings().root, false); refresh(); };
        luckyBtn.onClick = [this] { proc.setLucky (! proc.getSettings().lucky); refresh(); };
        for (auto* b : { &minorBtn, &majorBtn, &luckyBtn }) addAndMakeVisible (b);

        styleSeg (bars8, "8 bar melodies");
        styleSeg (bars16, "16 bar melodies");
        bars8.onClick = [this] { proc.setBars (8); refresh(); };
        bars16.onClick = [this] { proc.setBars (16); refresh(); };
        addAndMakeVisible (bars8);
        addAndMakeVisible (bars16);

        const char* soundTips[] = { "Pure sine beeps", "Classic square-wave chip beeps", "Soft triangle test tone" };
        for (int i = 0; i < 3; ++i)
        {
            styleSeg (sound[i], soundTips[i]);
            sound[i].onClick = [this, i] { proc.setSound (i); refresh(); };
            addAndMakeVisible (sound[i]);
        }

        generate.setTooltip ("12 new melodies in the chosen key, tempo and length");
        generate.onClick = [this]
        {
            proc.generate();
            refresh();
            auto set = proc.getSet();
            if (set != nullptr) flash ("12 fresh melodies in " + juce::String (ln::keyDisplayName (set->settings.key)));
        };
        addAndMakeVisible (generate);

        for (int i = 0; i < ln::kNumMelodies; ++i)
        {
            rows[(size_t) i] = std::make_unique<MelodyRow> (p, i, [this] (const juce::String& m) { flash (m); });
            addAndMakeVisible (*rows[(size_t) i]);
        }

        exportAll.getProperties().set ("ln-style", "outline");
        exportAll.setTooltip ("Save all 12 melodies as .mid files into a folder");
        exportAll.onClick = [this] { exportAllMidi(); };
        addAndMakeVisible (exportAll);

        styleSeg (withChords, "Also save a _Chords.mid (simple sustained triads) next to every melody");
        withChords.onClick = [this] { withChords.setToggleState (! withChords.getToggleState(), juce::dontSendNotification); };
        withChords.setToggleState (true, juce::dontSendNotification);
        addAndMakeVisible (withChords);

        refresh();
    }

    void resized() override
    {
        tempo.setBounds (36, 154, 196, 62);
        sync.setBounds (36, 224, 196, 28);

        key.setBounds (269, 154, 150, 62);
        up.setBounds (425, 154, 60, 29);
        down.setBounds (425, 187, 60, 29);
        minorBtn.setBounds (269, 224, 52, 28);
        majorBtn.setBounds (325, 224, 52, 28);
        luckyBtn.setBounds (381, 224, 104, 28);

        bars8.setBounds (522, 154, 96, 46);
        bars16.setBounds (522, 206, 96, 46);

        for (int i = 0; i < 3; ++i) sound[i].setBounds (655, 154 + i * 33, 126, 29);

        generate.setBounds (806, 132, 170, 130);

        for (int i = 0; i < ln::kNumMelodies; ++i)
            rows[(size_t) i]->setBounds (36, 304 + i * 30, 940, 29);

        withChords.setBounds (640, 676, 128, 32);
        exportAll.setBounds (776, 676, 200, 32);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (col::bg);

        // header: red logo square with the pixel clover, wordmark, explainer, slogan
        auto logo = juce::Rectangle<float> (24, 22, 72, 72);
        g.setColour (col::red.withAlpha (0.18f));
        g.fillRect (logo.expanded (3));
        g.setColour (col::red);
        g.fillRect (logo);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (logo.removeFromBottom (4).withX (logo.getX()));
        drawPixelClover (g, juce::Rectangle<float> (24, 22, 72, 72).reduced (9), col::green, true);

        g.setFont (pixelFont (9.0f));
        g.setColour (col::dim);
        g.drawText ("TRAPWIN", juce::Rectangle<float> (114, 26, 200, 12), juce::Justification::centredLeft, false);
        drawGlowText (g, "LUCKY NOTES", { 112, 42, 560, 36 }, juce::Justification::centredLeft, col::green, pixelFont (30.0f), 1.3f);
        g.setFont (monoFont (13.0f));
        g.setColour (col::text);
        g.drawText ("Random melodies that actually fit. Drag the MIDI into your DAW and give it your own sound.",
                    juce::Rectangle<float> (114, 82, 640, 18), juce::Justification::centredLeft, false);

        drawGlowText (g, "YOUR MIDI.", { 640, 30, 336, 20 }, juce::Justification::centredRight, col::green, pixelFont (14.0f), 1.0f);
        drawGlowText (g, "YOUR WORLD.", { 640, 54, 336, 20 }, juce::Justification::centredRight, col::green, pixelFont (14.0f), 1.0f);
        g.setFont (monoFont (11.0f));
        g.setColour (col::dim);
        g.drawText ("v" LN_VERSION_STRING + dot + "TRAPWIN", juce::Rectangle<float> (640, 82, 336, 18), juce::Justification::centredRight, false);

        g.setColour (col::border);
        g.fillRect (24.0f, 114.0f, 952.0f, 1.0f);

        // control panels
        drawPanel (g, { 24, 132, 220, 130 }, "TEMPO");
        drawPanel (g, { 257, 132, 240, 130 }, "KEY");
        drawPanel (g, { 510, 132, 120, 130 }, "LENGTH");
        drawPanel (g, { 643, 132, 150, 130 }, "PREVIEW SOUND");

        // melodies panel + column heads
        drawPanel (g, { 24, 276, 952, 392 }, {});
        g.setFont (pixelFont (8.0f));
        g.setColour (col::dim);
        const float hy = 287;
        g.drawText ("#", juce::Rectangle<float> (40, hy, 30, 10), juce::Justification::centredLeft, false);
        g.drawText ("PLAY", juce::Rectangle<float> (72, hy, 50, 10), juce::Justification::centredLeft, false);
        g.drawText ("CHARACTER", juce::Rectangle<float> (116, hy, 110, 10), juce::Justification::centredLeft, false);
        const auto s = proc.getSettings();
        g.drawText ("NOTES " + juce::String (s.bars) + " BARS", juce::Rectangle<float> (240, hy, 300, 10), juce::Justification::centredLeft, false);
        g.drawText ("ROLL", juce::Rectangle<float> (790, hy, 40, 10), juce::Justification::centredLeft, false);
        g.drawText ("TO YOUR DAW", juce::Rectangle<float> (832, hy, 140, 10), juce::Justification::centredLeft, false);
        g.setColour (col::border);
        g.fillRect (36.0f, 300.0f, 928.0f, 1.0f);

        // footer status line
        auto statusArea = juce::Rectangle<float> (28, 676, 600, 32);
        g.setColour (col::green);
        g.fillEllipse (statusArea.getX() + 2, statusArea.getCentreY() - 3, 6, 6);
        g.setColour (col::green.withAlpha (0.2f));
        g.fillEllipse (statusArea.getX() - 1, statusArea.getCentreY() - 6, 12, 12);
        g.setFont (monoFont (13.0f));
        const bool flashing = flashText.isNotEmpty() && juce::Time::getMillisecondCounter() < flashUntil;
        g.setColour (flashing ? col::green : col::text.withAlpha (0.85f));
        g.drawText (flashing ? flashText : statusText, statusArea.withTrimmedLeft (18), juce::Justification::centredLeft, true);
    }

    void paintOverChildren (juce::Graphics& g) override { drawScanlines (g, getLocalBounds()); }

    void refresh()
    {
        const auto s = proc.getSettings();
        const auto set = proc.getSet();
        tempo.update();
        key.update();
        sync.setToggleState (s.sync, juce::dontSendNotification);
        minorBtn.setToggleState (s.minor && ! s.lucky, juce::dontSendNotification);
        majorBtn.setToggleState (! s.minor && ! s.lucky, juce::dontSendNotification);
        luckyBtn.setToggleState (s.lucky, juce::dontSendNotification);
        bars8.setToggleState (s.bars == 8, juce::dontSendNotification);
        bars16.setToggleState (s.bars == 16, juce::dontSendNotification);
        for (int i = 0; i < 3; ++i) sound[i].setToggleState (s.sound == i, juce::dontSendNotification);
        if (set != nullptr && set != shownSet)
        {
            shownSet = set;
            for (int i = 0; i < ln::kNumMelodies; ++i) rows[(size_t) i]->setMelody (&set->melodies[(size_t) i]);
            repaint();
        }
        updateStatus();
        updatePlayheads();
    }

    void tick()
    {
        tempo.update();
        updateStatus();
        updatePlayheads();
        if (flashText.isNotEmpty() && juce::Time::getMillisecondCounter() >= flashUntil)
        {
            flashText.clear();
            repaint (0, 670, 640, 44);
        }
    }

    void flash (const juce::String& message)
    {
        flashText = message;
        flashUntil = juce::Time::getMillisecondCounter() + 3500;
        repaint (0, 670, 640, 44);
    }

private:
    void updateStatus()
    {
        const auto set = proc.getSet();
        if (set == nullptr) return;
        const auto t = "SEED " + juce::String::toHexString ((juce::int64) set->settings.seed).toUpperCase().paddedLeft ('0', 8)
                     + dot + formatBpm (proc.getEffectiveBpm()) + " BPM" + dot + juce::String (ln::keyDisplayName (set->settings.key))
                     + dot + juce::String (set->settings.bars) + " BARS";
        if (t != statusText) { statusText = t; repaint (0, 670, 640, 44); }
    }

    void updatePlayheads()
    {
        const int playingRow = proc.getPreviewRow();
        const double beat = proc.getPreviewBeat();
        for (int i = 0; i < ln::kNumMelodies; ++i)
            rows[(size_t) i]->setPlaying (i == playingRow, beat);
    }

    void exportAllMidi()
    {
        auto start = lastFolder.isDirectory() ? lastFolder : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
        chooser = std::make_unique<juce::FileChooser> ("Choose a folder for the 12 MIDI files", start, juce::String(), true);
        juce::Component::SafePointer<LuckyNotesContent> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [safe] (const juce::FileChooser& fc)
        {
            if (safe == nullptr) return;
            const auto dir = fc.getResult();
            if (dir == juce::File()) return;
            const bool chords = safe->withChords.getToggleState();
            int written = 0;
            for (int i = 0; i < ln::kNumMelodies; ++i)
            {
                if (safe->proc.writeMidiToFolder (i, dir).existsAsFile()) ++written;
                if (chords) safe->proc.writeMidiToFolder (i, dir, true);
            }
            safe->lastFolder = dir;
            safe->flash (juce::String (written) + " MIDI files saved to " + dir.getFullPathName());
        });
    }

    LuckyNotesProcessor& proc;
    TempoLcd tempo;
    KeyLcd key;
    juce::TextButton sync { "SYNC TO PROJECT" };
    juce::TextButton minorBtn { "MIN" }, majorBtn { "MAJ" }, luckyBtn { "LUCKY" };
    juce::TextButton bars8 { "8 BARS" }, bars16 { "16 BARS" };
    juce::TextButton sound[3] { juce::TextButton ("MORSE"), juce::TextButton ("8-BIT"), juce::TextButton ("TV TEST") };
    GenerateButton generate;
    IconButton up, down;
    std::array<std::unique_ptr<MelodyRow>, ln::kNumMelodies> rows;
    juce::TextButton exportAll { "EXPORT ALL MIDI" };
    juce::TextButton withChords { "+ CHORDS" };
    std::shared_ptr<const ln::MelodySet> shownSet;
    juce::String statusText, flashText;
    juce::uint32 flashUntil = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastFolder;
};

// ===============================================================================================================
LuckyNotesEditor::LuckyNotesEditor (LuckyNotesProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    content = std::make_unique<LuckyNotesContent> (p, lnf);
    content->setBounds (0, 0, kBaseWidth, kBaseHeight);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (kBaseWidth * 8 / 10, kBaseHeight * 8 / 10, kBaseWidth * 3 / 2, kBaseHeight * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);
    const double scale = juce::jlimit (0.8, 1.5, p.getUiScale());
    setSize (juce::roundToInt (kBaseWidth * scale), juce::roundToInt (kBaseHeight * scale));
    startTimerHz (30);
}

LuckyNotesEditor::~LuckyNotesEditor()
{
    stopTimer();
    content = nullptr;
    setLookAndFeel (nullptr);
}

void LuckyNotesEditor::paint (juce::Graphics& g) { g.fillAll (col::bg); }

void LuckyNotesEditor::resized()
{
    const float scale = juce::jlimit (0.5f, 2.0f, (float) getWidth() / (float) kBaseWidth);
    content->setTransform (juce::AffineTransform::scale (scale));
    proc.setUiScale (scale);
}

void LuckyNotesEditor::refreshNow()
{
    content->refresh();
    content->tick();
}

void LuckyNotesEditor::timerCallback()
{
    const int v = proc.getStateVersion();
    if (v != seenVersion) { seenVersion = v; content->refresh(); }
    content->tick();
}
