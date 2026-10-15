#pragma once
#include "VaultControls.h"
#include "VaultBrowserModel.h"
#include <juce_audio_utils/juce_audio_utils.h>

// CHROME VAULT UI building blocks of the browser "screen" (v3 mockup): the category column, the virtualised preset list,
// the ◀ ▶ buttons, the drag-out pill and the on-screen keyboard. All painting is cheap vector drawing.
namespace vui
{
//==============================================================================
// square arrow button (◀ ▶ around the search field)
class ArrowButton : public juce::Button
{
public:
    explicit ArrowButton (bool right) : juce::Button (right ? "Next sound" : "Previous sound"), toRight (right)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? col::rowHover : over ? juce::Colour (0xff0e1828) : juce::Colour (0xff070b11));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? col::ice2.withAlpha (0.7f) : col::pillLine);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        g.setColour (col::ice);
        g.fillPath (trianglePath (juce::Rectangle<float> (7.0f, 9.0f).withCentre (r.getCentre().translated (toRight ? 0.5f : -0.5f, 0.0f)), toRight));
    }

private:
    bool toRight;
};

//==============================================================================
// a pill that is dragged out of the plugin (DRAG WAV / DRAG MIDI): a drag renders the file and hands it to the host
class DragOutPill : public Pill
{
public:
    std::function<void()> onDragOut;      // once per drag gesture (performs the external drag)
    std::function<void()> onClickOnly;
    explicit DragOutPill (const juce::String& t) : Pill (t, Pill::Icon::down) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void mouseDown (const juce::MouseEvent& e) override { dragged = false; Pill::mouseDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        Pill::mouseDrag (e);
        if (! dragged && e.getDistanceFromDragStart() > 5) { dragged = true; if (onDragOut) onDragOut(); }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        Pill::mouseUp (e);
        if (! dragged && onClickOnly && getLocalBounds().contains (e.getPosition())) onClickOnly();
    }

private:
    bool dragged = false;
};

//==============================================================================
// "All Sounds" + the 16 categories with counts
class CategoryList : public juce::Component
{
public:
    std::function<void (int)> onSelect;   // -1 = All Sounds
    explicit CategoryList (const VaultBrowserModel& m) : model (m) { noFocus (*this); setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void setSelected (int c) { if (c != selected) { selected = c; repaint(); } }
    int getSelected() const { return selected; }
    int rowH = 22;
    static constexpr int padTop = 4;
    static juce::Font nameFont (bool on) { return font (14.0f, on); }
    juce::Rectangle<int> rowRect (int i) const { return { 0, padTop + i * rowH, getWidth(), rowH }; }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i <= vault::numCategories; ++i)
        {
            const int cat = i - 1;
            auto r = rowRect (i).toFloat();
            const bool on = cat == selected, over = i == hover;
            if (on)
            {
                g.setColour (col::catOn);
                g.fillRect (r);
                g.setColour (col::ice);
                g.fillRect (r.getX(), r.getY(), 3.0f, r.getHeight());
            }
            else if (over) { g.setColour (col::catHover); g.fillRect (r); }
            auto t = r.reduced (12.0f, 0.0f);
            const auto name = cat < 0 ? juce::String ("All Sounds") : vault::categoryNames()[cat];
            const auto cnt = thousands (model.categoryCount (cat));
            const auto cf = font (14.0f);
            const float cw = textWidth (cf, cnt) + 2.0f;
            g.setFont (cf);
            g.setColour (on ? col::ice : col::dim);
            g.drawText (cnt, t.removeFromRight (cw), juce::Justification::centredRight, false);
            g.setFont (nameFont (on));
            g.setColour (on ? col::silver : over ? col::text : col::muted);
            g.drawText (name, t.withTrimmedRight (6.0f), juce::Justification::centredLeft, true);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { setHover (rowAt (e.y)); }
    void mouseExit (const juce::MouseEvent&) override { setHover (-1); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = rowAt (e.y);
        if (i >= 0 && onSelect) onSelect (i - 1);
    }
    // tests: the widest row (name + count, both paddings) must fit the column
    float widestRow() const
    {
        float w = 0;
        for (int i = 0; i <= vault::numCategories; ++i)
        {
            const auto name = i == 0 ? juce::String ("All Sounds") : vault::categoryNames()[i - 1];
            w = std::max (w, textWidth (nameFont (true), name) + 6.0f + textWidth (font (14.0f), thousands (model.categoryCount (i - 1))) + 2.0f + 24.0f);
        }
        return w;
    }

private:
    int rowAt (int y) const
    {
        for (int i = 0; i <= vault::numCategories; ++i) if (rowRect (i).contains (4, y)) return i;
        return -1;
    }
    void setHover (int h) { if (h != hover) { hover = h; repaint(); } }

    const VaultBrowserModel& model;
    int selected = -1, hover = -1;
};

//==============================================================================
// the preset list: juce::ListBox (only the visible rows exist - smooth with 5,000 rows). Row: name | "3 layers" | heart
class PresetList : public juce::Component, private juce::ListBoxModel, private juce::ComponentListener
{
public:
    std::function<void (int bankIndex)> onClick;           // load + play
    std::function<void (int bankIndex)> onToggleFavorite;
    static constexpr int rowH = 27;

    PresetList (const std::vector<VaultPreset>& b, VaultBrowserModel& m) : bank (b), model (m)
    {
        noFocus (*this);
        list.setModel (this);
        list.setRowHeight (rowH);
        list.setOutlineThickness (0);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.getViewport()->setScrollBarThickness (8);
        list.getViewport()->setSingleStepSizes (rowH, rowH);
        list.setWantsKeyboardFocus (false);
        list.setMouseMoveSelectsRows (false);
        addAndMakeVisible (list);
        list.addMouseListener (this, true);
        if (auto* content = list.getViewport()->getViewedComponent()) content->addComponentListener (this);
        muteRowFocus();
    }
    ~PresetList() override
    {
        if (auto* content = list.getViewport()->getViewedComponent()) content->removeComponentListener (this);
        list.removeMouseListener (this);
    }

    void setCurrent (int bankIndex) { if (bankIndex != current) { current = bankIndex; list.repaint(); } }
    // the list's row components (made by juce::ListBox as it scrolls) never take the keyboard on a click
    void muteRowFocus()
    {
        std::function<void (juce::Component&)> mute = [&] (juce::Component& c)
        {
            c.setWantsKeyboardFocus (false);
            c.setMouseClickGrabsKeyboardFocus (false);
            for (auto* ch : c.getChildren()) mute (*ch);
        };
        mute (list);
    }
    void refresh() { list.updateContent(); muteRowFocus(); list.repaint(); }
    void showRow (int row) { if (row >= 0) list.scrollToEnsureRowIsOnscreen (row); }
    void scrollToTop() { list.getViewport()->setViewPosition (0, 0); }
    juce::ListBox& listBox() { return list; }

    void resized() override { list.setBounds (getLocalBounds().reduced (1)); muteRowFocus(); }
    void paint (juce::Graphics& g) override
    {
        if (! model.visible().empty()) return;
        g.setColour (col::dim);
        g.setFont (font (15.0f));
        g.drawFittedText (model.filter.favoritesOnly && model.numFavorites() == 0
                              ? "No favorites yet. Click the heart next to a sound to keep it here."
                              : model.filter.recentOnly && model.recentList().empty()
                                    ? "Nothing played yet. Click a sound - it shows up here."
                                    : "No sounds found. Try another word or another category.",
                          getLocalBounds().reduced (16, 14).removeFromTop (44), juce::Justification::topLeft, 2);
    }

    // row columns (public for the readability test: every name of the bank must fit without "...")
    struct Cols { juce::Rectangle<float> name, badge, heart; };
    static Cols columns (float w, float h)
    {
        juce::Rectangle<float> r (14.0f, 0.0f, w - 24.0f, h);
        Cols c;
        c.heart = r.removeFromRight (18.0f); r.removeFromRight (10.0f);
        c.badge = r.removeFromRight (62.0f); r.removeFromRight (8.0f);
        c.name = r;
        return c;
    }
    static juce::Font nameFont() { return font (15.0f); }
    static juce::Font tagFont()  { return font (14.0f); }
    // the main instrument(s) of a preset as one line ("Concert Grand · Violin Section"), as many as fit (never cut)
    static juce::String infoText (const juce::StringArray& instruments, float maxWidth)
    {
        juce::String out;
        for (auto& s : instruments)
        {
            const auto next = out.isEmpty() ? s : out + midDot() + s;
            if (out.isNotEmpty() && textWidth (tagFont(), next) > maxWidth) break;
            out = next;
        }
        return out;
    }

private:
    void componentChildrenChanged (juce::Component&) override { muteRowFocus(); }
    int getNumRows() override { return (int) model.visible().size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (row, (int) model.visible().size())) return;
        const int idx = model.visible()[(size_t) row];
        const auto& p = bank[(size_t) idx];
        const bool sel = idx == current, over = row == hoverRow;
        const auto fw = (float) w, fh = (float) h;
        if (sel)
        {
            g.setGradientFill (juce::ColourGradient (col::selA, 0, 0, col::selB, fw, 0, false));
            g.fillRect (0.0f, 0.0f, fw, fh);
        }
        else if (over) { g.setColour (col::rowHover); g.fillRect (0.0f, 0.0f, fw, fh); }
        g.setColour (col::wellRow);
        g.fillRect (0.0f, fh - 1.0f, fw, 1.0f);

        const auto c = columns (fw, fh);
        g.setFont (nameFont());
        g.setColour (sel ? col::silver : col::text);
        g.drawText (p.name, c.name, juce::Justification::centredLeft, true);
        const int n = (int) p.parts.size();
        g.setFont (tagFont());
        g.setColour (col::dim);
        g.drawText (juce::String (n) + (n == 1 ? " layer" : " layers"), c.badge, juce::Justification::centredRight, false);
        const bool fav = model.isFavorite (idx);
        g.setColour (fav ? col::ice : (over && hoverHeart) ? col::muted : col::heartOff);
        g.fillPath (heartPath (juce::Rectangle<float> (13.0f, 13.0f).withCentre (c.heart.getCentre())));
    }

    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (! juce::isPositiveAndBelow (row, (int) model.visible().size())) return;
        const int idx = model.visible()[(size_t) row];
        const auto c = columns ((float) list.getVisibleRowWidth(), (float) rowH);
        if (c.heart.expanded (8.0f, 0.0f).contains ((float) e.x, (float) rowH * 0.5f)) { if (onToggleFavorite) onToggleFavorite (idx); }
        else if (onClick) onClick (idx);
    }

    void mouseMove (const juce::MouseEvent& e) override { updateHover (e); }
    void mouseDrag (const juce::MouseEvent& e) override { updateHover (e); }
    void mouseExit (const juce::MouseEvent& e) override
    {
        if (! list.getScreenBounds().contains (e.getScreenPosition())) setHover (-1, false);
    }
    void updateHover (const juce::MouseEvent& e)
    {
        const auto p = e.getEventRelativeTo (&list).getPosition();
        const int row = list.getRowContainingPosition (p.x, p.y);
        const auto c = columns ((float) list.getVisibleRowWidth(), (float) rowH);
        setHover (row, c.heart.expanded (8.0f, 0.0f).getHorizontalRange().contains ((float) p.x));
    }
    void setHover (int row, bool heart)
    {
        if (row == hoverRow && heart == hoverHeart) return;
        const int old = hoverRow;
        hoverRow = row; hoverHeart = heart;
        if (old >= 0) list.repaintRow (old);
        if (row >= 0) list.repaintRow (row);
    }

    const std::vector<VaultPreset>& bank;
    VaultBrowserModel& model;
    juce::ListBox list;
    int current = -1, hoverRow = -1;
    bool hoverHeart = false;
};

//==============================================================================
// on-screen keyboard: silver keys, ice when pressed, C labels in FL Studio naming. MIDI 60 = C5 - the home note every
// sound is designed and previewed at - is the ice key.
class VaultKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit VaultKeyboard (juce::MidiKeyboardState& s) : juce::MidiKeyboardComponent (s, horizontalKeyboard)
    {
        setOctaveForMiddleC (vault::octaveForMiddleC);   // MIDI 60 = C5 (FL Studio)
        setScrollButtonsVisible (false);
        setAvailableRange (36, 83);   // C3..B6 (FL naming), 4 octaves, C5 = MIDI 60 in the third octave
        setLowestVisibleKey (36);
        setBlackNoteLengthProportion (0.6f);
        setBlackNoteWidthProportion (0.55f);
        setVelocity (0.8f, true);
        setWantsKeyboardFocus (false);               // the DAW keeps the computer keyboard
        setMouseClickGrabsKeyboardFocus (false);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setColour (whiteNoteColourId, col::keyBed);
        setColour (shadowColourId, juce::Colours::transparentBlack);
        setColour (keySeparatorLineColourId, juce::Colours::transparentBlack);
    }
    void resized() override
    {
        setKeyWidth ((float) getWidth() / 28.0f);
        juce::MidiKeyboardComponent::resized();
    }

protected:
    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> a, bool isDown, bool isOver, juce::Colour, juce::Colour) override
    {
        auto k = a.reduced (0.5f, 0.0f);
        const bool home = note == vault::homeNote;
        juce::Path p;
        p.addRoundedRectangle (k.getX(), k.getY() - 4.0f, k.getWidth(), k.getHeight() + 4.0f, 4.0f, 4.0f, false, false, true, true);
        if (isDown) g.setGradientFill (juce::ColourGradient (col::ice2, 0, k.getY(), col::deep, 0, k.getBottom(), false));
        else if (home) g.setGradientFill (juce::ColourGradient (col::ice, 0, k.getY(), col::ice2, 0, k.getBottom(), false));
        else g.setGradientFill (juce::ColourGradient (juce::Colour (isOver ? 0xfff4f7fc : 0xffe6eaf1), 0, k.getY(), juce::Colour (0xffc5ccd8), 0, k.getBottom(), false));
        g.fillPath (p);
        if (note % 12 == 0)
        {
            g.setColour (juce::Colour (home || isDown ? 0xff0b0f16 : 0xff56627a));
            g.setFont (font (14.0f, home));
            g.drawText (vault::noteName (note), k.removeFromBottom (19.0f), juce::Justification::centred, false);
        }
    }
    void drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> a, bool isDown, bool isOver, juce::Colour) override
    {
        juce::Path p;
        p.addRoundedRectangle (a.getX(), a.getY() - 4.0f, a.getWidth(), a.getHeight() + 4.0f, 3.0f, 3.0f, false, false, true, true);
        if (isDown) g.setGradientFill (juce::ColourGradient (col::deep, 0, a.getY(), juce::Colour (0xff1d3561), 0, a.getBottom(), false));
        else g.setGradientFill (juce::ColourGradient (juce::Colour (isOver ? 0xff2e3849 : 0xff232b3a), 0, a.getY(), juce::Colour (0xff05070b), 0, a.getBottom(), false));
        g.fillPath (p);
    }
    juce::String getWhiteNoteText (int) override { return {}; }
};

//==============================================================================
// what plays a layer, as a small vector icon: sample = waveform bars, model = a sine (a modelled, vibrating source),
// synth = a saw wave
inline void drawKindIcon (juce::Graphics& g, juce::Rectangle<float> r, VaultPartKind kind, juce::Colour c)
{
    g.setColour (c);
    const float cy = r.getCentreY(), h = r.getHeight();
    if (kind == VaultPartKind::sample)
    {
        static constexpr float hs[] { 0.36f, 0.78f, 1.0f, 0.56f, 0.3f };
        const float w = r.getWidth() / 5.0f;
        for (int i = 0; i < 5; ++i)
        {
            const float bh = h * hs[i];
            g.fillRoundedRectangle (r.getX() + (float) i * w + w * 0.18f, cy - bh * 0.5f, w * 0.64f, bh, w * 0.3f);
        }
        return;
    }
    juce::Path p;
    if (kind == VaultPartKind::model)
    {
        for (int i = 0; i <= 28; ++i)
        {
            const float t = (float) i / 28.0f, x = r.getX() + t * r.getWidth();
            const float y = cy - std::sin (t * juce::MathConstants<float>::twoPi) * h * 0.36f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
    }
    else
    {
        const float x = r.getX(), w = r.getWidth(), a = h * 0.36f;
        p.startNewSubPath (x, cy + a);
        p.lineTo (x + w * 0.5f, cy - a);
        p.lineTo (x + w * 0.5f, cy + a);
        p.lineTo (x + w, cy - a);
    }
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// a quiet "working" indicator: a faint ring with a turning arc (time-based; the caller repaints while it is shown)
inline void drawSpinner (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const float d = std::min (r.getWidth(), r.getHeight()), rad = d * 0.5f - 1.0f;
    const auto m = r.getCentre();
    const float ph = (float) (juce::Time::getMillisecondCounter() % 1100u) / 1100.0f * juce::MathConstants<float>::twoPi;
    g.setColour (c.withAlpha (0.22f));
    g.drawEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (m), 1.6f);
    juce::Path p;
    p.addCentredArc (m.x, m.y, rad, rad, 0.0f, ph, ph + 2.2f, true);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// greedy word wrap (no shrinking, no "..."): breaks at spaces and, for file paths, after '/' or '\'
inline juce::StringArray wrapText (const juce::Font& f, const juce::String& text, float width)
{
    juce::StringArray tokens;   // each token keeps its trailing separator
    juce::String cur;
    for (auto ch : text)
    {
        cur << juce::String::charToString (ch);
        if (ch == ' ' || ch == '/' || ch == '\\') { tokens.add (cur); cur.clear(); }
    }
    if (cur.isNotEmpty()) tokens.add (cur);
    juce::StringArray lines;
    juce::String line;
    for (auto& t : tokens)
    {
        if (line.isNotEmpty() && textWidth (f, (line + t).trimEnd()) > width) { lines.add (line.trimEnd()); line.clear(); }
        line << t;
    }
    if (line.trim().isNotEmpty()) lines.add (line.trimEnd());
    return lines;
}
} // namespace vui
