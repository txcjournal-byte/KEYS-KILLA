#pragma once
#include "VaultTheme.h"
#include "VaultBrowserModel.h"
#include <juce_audio_utils/juce_audio_utils.h>

// CHROME VAULT UI building blocks: pill buttons, the category list, filter chips, the virtualised preset list,
// the on-screen keyboard and the "DRAG WAV" pill. All painting is cheap vector drawing (no work in paint beyond text).
namespace vui
{
//==============================================================================
// rounded pill button (top bar / status bar). Toggle "on" = ice outline + ice text, like the mockup.
class PillButton : public juce::Button
{
public:
    enum class Icon { none, heart, dice, clock };
    PillButton (const juce::String& text, Icon i = Icon::none, float fontPx = 14.0f) : juce::Button (text), icon (i), px (fontPx)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        g.setColour (down ? col::rowHover : col::btn);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? col::ice : over ? col::ice2 : col::btnLine);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, on ? 1.4f : 1.0f);
        const auto f = font (px, true, 0.04f);
        const auto txt = getButtonText();
        const float iconW = icon == Icon::none ? 0.0f : px + 6.0f;
        const float w = iconW + textWidth (f, txt);
        float x = r.getCentreX() - w * 0.5f;
        const auto c = on ? col::ice : col::silver;
        if (icon != Icon::none)
        {
            auto ir = juce::Rectangle<float> (x, r.getCentreY() - px * 0.5f, px, px);
            if (icon == Icon::heart) { g.setColour (on ? col::ice : col::muted); g.fillPath (heartPath (ir)); }
            if (icon == Icon::dice) drawDice (g, ir.reduced (0.5f), on ? col::ice : col::muted);
            if (icon == Icon::clock) drawClock (g, ir.reduced (0.5f), on ? col::ice : col::muted);
            x += iconW;
        }
        g.setColour (c);
        g.setFont (f);
        g.drawText (txt, juce::Rectangle<float> (x, r.getY(), w - iconW + 2.0f, r.getHeight()), juce::Justification::centredLeft, false);
    }
    int idealWidth() const { return (int) std::ceil (textWidth (font (px, true, 0.04f), getButtonText()) + (icon == Icon::none ? 0.0f : px + 6.0f) + 26.0f); }

private:
    Icon icon;
    float px;
};

// small round arrow button (◀ ▶ around the current preset name)
class ArrowButton : public juce::Button
{
public:
    explicit ArrowButton (bool right) : juce::Button (right ? "Next sound" : "Previous sound"), toRight (right)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        if (over || down) { g.setColour (juce::Colour (0xff132036)); g.fillEllipse (r.reduced (1.0f)); }
        const float s = r.getHeight() * 0.3f;
        g.setColour (col::ice);
        g.fillPath (trianglePath (juce::Rectangle<float> (s * 0.85f, s).withCentre (r.getCentre().translated (toRight ? 1.0f : -1.0f, 0.0f)), toRight));
    }

private:
    bool toRight;
};

//==============================================================================
// left column: "All Sounds" + the 16 categories with counts
class CategoryList : public juce::Component
{
public:
    std::function<void (int)> onSelect;   // -1 = All Sounds
    explicit CategoryList (const VaultBrowserModel& m) : model (m) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void setSelected (int c) { if (c != selected) { selected = c; repaint(); } }
    static constexpr int rowH = 30, gap = 1, padTop = 12;

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i <= vault::numCategories; ++i)
        {
            const int cat = i - 1;
            auto r = rowRect (i).toFloat();
            const bool on = cat == selected, over = i == hover;
            if (on)
            {
                g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (0.18f), r.getX(), 0, col::ice.withAlpha (0.02f), r.getRight(), 0, false));
                g.fillRoundedRectangle (r, 10.0f);
            }
            else if (over)
            {
                g.setColour (col::catHover);
                g.fillRoundedRectangle (r, 10.0f);
            }
            auto t = r.reduced (12.0f, 0.0f);
            const auto name = cat < 0 ? juce::String ("All Sounds") : vault::categoryNames()[cat];
            const auto cnt = thousands (model.categoryCount (cat));
            const auto cf = font (14.0f, on);
            const float cw = textWidth (cf, cnt) + 2.0f;
            g.setFont (cf);
            g.setColour (on ? col::ice : col::dim);
            g.drawText (cnt, t.removeFromRight (cw), juce::Justification::centredRight, false);
            g.setFont (font (14.0f, on || cat < 0));
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

private:
    juce::Rectangle<int> rowRect (int i) const
    {
        const int extra = i > 0 ? 6 : 0;   // a little air under "All Sounds"
        return { 8, padTop + i * (rowH + gap) + extra, getWidth() - 16, rowH };
    }
    int rowAt (int y) const
    {
        for (int i = 0; i <= vault::numCategories; ++i) if (rowRect (i).expanded (0, 1).contains (12, y)) return i;
        return -1;
    }
    void setHover (int h) { if (h != hover) { hover = h; repaint(); } }

    const VaultBrowserModel& model;
    int selected = -1, hover = -1;
};

//==============================================================================
// filter chips: LAYERS (All 1 2 3 4) + "Clear filters"
class ChipBar : public juce::Component
{
public:
    std::function<void()> onChange;
    explicit ChipBar (VaultBrowserFilter& f) : filter (f) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    static constexpr int labelW = 106, chipH = 26, rowPitch = 31, padTop = 10;
    int idealHeight() const { return padTop + chipH + 10; }

    void resized() override
    {
        chips.clear();
        auto add = [&] (int group, int value, const juce::String& txt, int rowIdx, int& x)
        {
            const int w = (int) std::ceil (textWidth (font (14.0f, true), txt)) + 18;
            chips.push_back ({ group, value, txt, { x, padTop + rowIdx * rowPitch, w, chipH } });
            x += w + 5;
        };
        int x = 14 + labelW;
        add (2, 0, "All", 0, x);
        for (int i = 1; i <= vault::maxParts; ++i) add (2, i, juce::String (i), 0, x);
        const auto ct = juce::String ("Clear filters");
        const int cw = (int) std::ceil (textWidth (font (14.0f), ct)) + 18;
        clearRect = { getWidth() - 14 - cw, padTop, cw, chipH };
    }

    void paint (juce::Graphics& g) override
    {
        g.setFont (font (14.0f, true, 0.04f));
        g.setColour (col::dim);
        g.drawText ("LAYERS", 14, padTop, labelW, chipH, juce::Justification::centredLeft, false);
        for (int i = 0; i < (int) chips.size(); ++i)
        {
            const auto& c = chips[(size_t) i];
            const bool on = isOn (c), over = i == hover;
            auto r = c.r.toFloat().reduced (0.5f);
            if (on) { g.setColour (col::ice); g.fillRoundedRectangle (r, r.getHeight() * 0.5f); }
            else { g.setColour (over ? col::ice2 : col::fieldLine); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f); }
            g.setFont (font (14.0f, true));
            g.setColour (on ? col::ink : over ? col::text : col::muted);
            g.drawText (c.text, c.r, juce::Justification::centred, false);
        }
        if (filter.anyChip())
        {
            g.setFont (font (14.0f));
            g.setColour (hover == clearIndex ? col::ice : col::dim);
            g.drawText ("Clear filters", clearRect, juce::Justification::centred, false);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { setHover (hit (e.getPosition())); }
    void mouseExit (const juce::MouseEvent&) override { setHover (-1); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int h = hit (e.getPosition());
        if (h == clearIndex) filter.layers = 0;
        else if (h >= 0) filter.layers = chips[(size_t) h].value;
        else return;
        repaint();
        if (onChange) onChange();
    }

private:
    struct Chip { int group, value; juce::String text; juce::Rectangle<int> r; };
    static constexpr int clearIndex = 1000;
    bool isOn (const Chip& c) const
    {
        return filter.layers == c.value;
    }
    int hit (juce::Point<int> p) const
    {
        for (int i = 0; i < (int) chips.size(); ++i) if (chips[(size_t) i].r.contains (p)) return i;
        if (filter.anyChip() && clearRect.contains (p)) return clearIndex;
        return -1;
    }
    void setHover (int h) { if (h != hover) { hover = h; repaint(); } }

    VaultBrowserFilter& filter;
    std::vector<Chip> chips;
    juce::Rectangle<int> clearRect;
    int hover = -1;
};

//==============================================================================
// the preset list: juce::ListBox (only the visible rows exist - smooth with 5,000 rows).
// Columns: selected dot | name | main instrument(s) | layer badge | heart
class PresetList : public juce::Component, private juce::ListBoxModel, private juce::ComponentListener
{
public:
    std::function<void (int bankIndex)> onClick;           // load + play
    std::function<void (int bankIndex)> onToggleFavorite;
    static constexpr int rowH = 34;

    PresetList (const std::vector<VaultPreset>& b, VaultBrowserModel& m) : bank (b), model (m)
    {
        list.setModel (this);
        list.setRowHeight (rowH);
        list.setOutlineThickness (0);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.getViewport()->setScrollBarThickness (10);
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

    void resized() override { list.setBounds (getLocalBounds()); muteRowFocus(); }
    void paint (juce::Graphics& g) override
    {
        if (! model.visible().empty()) return;
        g.setColour (col::dim);
        g.setFont (font (15.0f));
        g.drawFittedText (model.filter.favoritesOnly && model.numFavorites() == 0
                              ? "No favorites yet. Click the heart next to a sound to keep it here."
                              : model.filter.recentOnly && model.recentList().empty()
                                    ? "Nothing played yet. Click a sound - it shows up here."
                                    : "No sounds found. Try another word or turn off a filter.",
                          getLocalBounds().reduced (20, 18).removeFromTop (40), juce::Justification::topLeft, 2);
    }

    // row columns (public for the readability test: every name and tag line of the bank must fit without "...")
    struct Cols { juce::Rectangle<float> dot, name, tags, badge, heart; };
    static Cols columns (float w, float h)
    {
        // name: the longest factory names are ~188 px at 15 px bold; the info line gets the rest (up to ~264 px)
        juce::Rectangle<float> r (12.0f, 0.0f, w - 24.0f, h);
        Cols c;
        c.dot = r.removeFromLeft (20.0f); r.removeFromLeft (8.0f);
        c.heart = r.removeFromRight (20.0f); r.removeFromRight (6.0f);
        c.badge = r.removeFromRight (92.0f); r.removeFromRight (8.0f);
        c.name = r.removeFromLeft (juce::jmax (190.0f, r.getWidth() - 8.0f - 264.0f)); r.removeFromLeft (8.0f);
        c.tags = r;
        return c;
    }
    static juce::Font nameFont() { return font (15.0f, true); }
    static juce::Font tagFont()  { return font (14.0f); }
    // the info line: the main instrument(s) of the preset ("Concert Grand · Violin Section"), as many as fit the column
    // (the first one always; further ones only whole - never cut with "...")
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
            g.setGradientFill (juce::ColourGradient (col::deep.withAlpha (0.35f), 0, 0, col::deep.withAlpha (0.06f), fw, 0, false));
            g.fillRect (0.0f, 0.0f, fw, fh);
        }
        else if (over) { g.setColour (col::rowHover); g.fillRect (0.0f, 0.0f, fw, fh); }
        g.setColour (col::rowLine);
        g.fillRect (0.0f, fh - 1.0f, fw, 1.0f);

        const auto c = columns (fw, fh);
        // dot
        const auto d = juce::Rectangle<float> (8.0f, 8.0f).withCentre (c.dot.getCentre());
        if (sel)
        {
            g.setColour (col::ice.withAlpha (0.25f)); g.fillEllipse (d.expanded (3.5f));
            g.setColour (col::ice); g.fillEllipse (d);
        }
        else { g.setColour (col::btnLine); g.fillEllipse (d); }
        // name
        g.setFont (nameFont());
        g.setColour (sel ? col::silver : col::text);
        g.drawText (p.name, c.name, juce::Justification::centredLeft, true);
        g.setFont (tagFont());
        g.setColour (col::dim);
        g.drawText (infoText (model.instruments (idx), c.tags.getWidth()), c.tags, juce::Justification::centredLeft, true);
        // layer badge
        const int n = (int) p.parts.size();
        auto b = c.badge;
        for (int i = 0; i < vault::maxParts; ++i)
        {
            auto bar = b.removeFromLeft (6.0f).withSizeKeepingCentre (6.0f, 14.0f);
            g.setColour (i < n ? col::ice2 : col::fieldLine);
            g.fillRoundedRectangle (bar, 2.0f);
            b.removeFromLeft (3.0f);
        }
        b.removeFromLeft (4.0f);
        g.setFont (font (14.0f));
        g.setColour (col::muted);
        g.drawText (juce::String (n) + (n == 1 ? " layer" : " layers"), b, juce::Justification::centredLeft, false);
        // heart
        const bool fav = model.isFavorite (idx);
        g.setColour (fav ? col::ice : (over && hoverHeart) ? col::muted : col::heartOff);
        g.fillPath (heartPath (juce::Rectangle<float> (15.0f, 15.0f).withCentre (c.heart.getCentre())));
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
// on-screen keyboard in the palette: silver keys, ice when pressed, C labels in FL Studio naming (MIDI 60 = C5, the home
// note every sound is designed and previewed at - drawn in ice with a marker)
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
        setBlackNoteWidthProportion (0.62f);
        setVelocity (0.8f, true);
        setWantsKeyboardFocus (false);               // the DAW keeps the computer keyboard
        setMouseClickGrabsKeyboardFocus (false);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setColour (whiteNoteColourId, col::field);              // background between the keys
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
        juce::Path p;
        p.addRoundedRectangle (k.getX(), k.getY() - 6.0f, k.getWidth(), k.getHeight() + 6.0f, 5.0f, 5.0f, false, false, true, true);
        if (isDown) g.setGradientFill (juce::ColourGradient (col::ice, 0, k.getY(), col::ice2, 0, k.getBottom(), false));
        else g.setGradientFill (juce::ColourGradient (juce::Colour (isOver ? 0xfff4f7fc : 0xffe9edf4), 0, k.getY(), juce::Colour (0xffc8cfdc), 0, k.getBottom(), false));
        g.fillPath (p);
        if (note % 12 == 0)
        {
            const bool home = note == vault::homeNote;
            g.setColour (juce::Colour (isDown ? 0xff1d3561 : home ? 0xff2f5fae : 0xff5a6478));
            g.setFont (font (home ? 15.0f : 14.0f, true));
            g.drawText (vault::noteName (note), k.removeFromBottom (22.0f), juce::Justification::centred, false);
            if (home)
            {
                // the home note: an ice dot above the label
                g.setColour (isDown ? juce::Colour (0xff1d3561) : col::deep);
                g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ k.getCentreX(), k.getBottom() - 6.0f }));
            }
        }
    }
    void drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> a, bool isDown, bool isOver, juce::Colour) override
    {
        juce::Path p;
        p.addRoundedRectangle (a.getX(), a.getY() - 4.0f, a.getWidth(), a.getHeight() + 4.0f, 3.5f, 3.5f, false, false, true, true);
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

//==============================================================================
// "DRAG WAV" pill:dragging it out renders the current sound to a WAV file and hands it to the DAW / desktop
class DragPill : public juce::Component
{
public:
    std::function<void()> onDragOut;   // called once per drag gesture (performs the external drag)
    std::function<void()> onClickOnly;
    DragPill() { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool over = isMouseOverOrDragging();
        g.setColour (col::btn);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (over ? col::ice : col::btnLine);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        const auto f = font (14.0f, true, 0.04f);
        const juce::String t ("DRAG WAV");
        const float w = 12.0f + 7.0f + textWidth (f, t);
        float x = r.getCentreX() - w * 0.5f;
        drawDownArrow (g, { x + 2.0f, r.getCentreY() - 6.0f, 8.0f, 12.0f }, over ? col::ice : col::muted);
        x += 19.0f;
        g.setColour (over ? col::ice : col::silver);
        g.setFont (f);
        g.drawText (t, juce::Rectangle<float> (x, r.getY(), w, r.getHeight()), juce::Justification::centredLeft, false);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDown (const juce::MouseEvent&) override { dragged = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragged && e.getDistanceFromDragStart() > 5) { dragged = true; if (onDragOut) onDragOut(); }
    }
    void mouseUp (const juce::MouseEvent&) override { if (! dragged && onClickOnly) onClickOnly(); repaint(); }

private:
    bool dragged = false;
};
} // namespace vui
