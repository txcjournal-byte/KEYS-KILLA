#include "VaultEditor.h"
#include "ui/VaultMainView.h"
#include <juce_audio_formats/juce_audio_formats.h>

using namespace vui;

namespace
{
juce::String layerWord (int n) { return n == 1 ? "layer" : "layers"; }

// remove any focus-on-click from a component tree (focus stays with the editor unless the search box is clicked)
void noClickFocus (juce::Component& c, const juce::Component* except)
{
    if (&c == except) return;
    c.setMouseClickGrabsKeyboardFocus (false);
    c.setWantsKeyboardFocus (false);
    for (auto* ch : c.getChildren()) noClickFocus (*ch, except);
}
} // namespace

//==============================================================================
// VaultInfoPanel
//==============================================================================
VaultInfoPanel::VaultInfoPanel (VaultProcessor& p, VaultBrowserModel& m, VaultThumbs& t) : proc (p), model (m), thumbs (t)
{
    for (int i = 0; i < vault::numMacros; ++i)
    {
        auto& k = knob[i];
        k.setName (vault::macroNames()[i]);
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        k.setRotaryParameters (juce::MathConstants<float>::pi * -0.75f, juce::MathConstants<float>::pi * 0.75f, true);
        k.setMouseDragSensitivity (220);
        k.setRepaintsOnMouseActivity (true);
        k.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        k.setTitle (vault::macroNames()[i]);
        addAndMakeVisible (k);
        knobAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, VaultProcessor::idMacro[i], k);
        k.onValueChange = [this] { repaint (waveR); };   // the waveform follows the macros
    }
    volume.setName ("VOLUME");
    volume.setTitle ("Volume");
    volume.setSliderStyle (juce::Slider::LinearHorizontal);
    volume.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    volume.setRepaintsOnMouseActivity (true);
    addAndMakeVisible (volume);
    volumeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, VaultProcessor::idVolume, volume);
    volume.setDoubleClickReturnValue (true, 0.0);
    volume.onValueChange = [this] { repaint (volumeR); };
    addAndMakeVisible (dragPill);
}

VaultInfoPanel::~VaultInfoPanel() = default;

void VaultInfoPanel::setPreset (int idx)
{
    preset = idx;
    if (const auto* p = juce::isPositiveAndBelow (idx, (int) proc.bank().size()) ? &proc.bank()[(size_t) idx] : nullptr)
        for (int i = 0; i < vault::numMacros; ++i) knob[i].setDoubleClickReturnValue (true, p->macroDefaults[i]);
    repaint();
}

void VaultInfoPanel::resized()
{
    auto r = getLocalBounds().reduced (18, 16);
    nameR = r.removeFromTop (28);
    r.removeFromTop (2);
    catR = r.removeFromTop (18);
    r.removeFromTop (8);
    tagsR = r.removeFromTop (50);
    r.removeFromTop (12);
    waveR = r.removeFromTop (58);
    r.removeFromTop (14);
    layersLabelR = r.removeFromTop (26);
    r.removeFromTop (8);
    stackR = r.removeFromTop (4 * 30 + 3 * 6);

    volumeR = r.removeFromBottom (26);
    r.removeFromBottom (12);
    knobsR = r.removeFromBottom (80);
    const int kw = knobsR.getWidth() / vault::numMacros;
    for (int i = 0; i < vault::numMacros; ++i) knob[i].setBounds (knobsR.getX() + i * kw, knobsR.getY(), kw, knobsR.getHeight());
    auto v = volumeR;
    v.removeFromLeft (66);
    v.removeFromRight (62);
    volume.setBounds (v);
    dragPill.setBounds (layersLabelR.withLeft (layersLabelR.getRight() - 108));
}

juce::Rectangle<float> VaultInfoPanel::heartRect() const
{
    return juce::Rectangle<float> (26.0f, 26.0f).withCentre ({ (float) nameR.getRight() - 11.0f, (float) nameR.getCentreY() });
}

void VaultInfoPanel::paint (juce::Graphics& g)
{
    using namespace juce;
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (col::infoA, 0, 0, col::infoB, 0, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (col::line);
    g.fillRect (0.0f, 0.0f, 1.0f, b.getHeight());

    const VaultPreset* p = isPositiveAndBelow (preset, (int) proc.bank().size()) ? &proc.bank()[(size_t) preset] : nullptr;
    if (p == nullptr)
    {
        g.setColour (col::dim);
        g.setFont (font (14.0f));
        g.drawText ("No sound loaded", nameR, Justification::centredLeft);
        return;
    }
    // name + heart
    g.setFont (font (20.0f, true));
    g.setColour (col::silver);
    g.drawText (p->name, nameR.withTrimmedRight (30), Justification::centredLeft, true);
    const bool fav = model.isFavorite (preset);
    g.setColour (fav ? col::ice : heartOver ? col::muted : col::heartOff);
    g.fillPath (heartPath (heartRect().reduced (5.0f)));

    const int n = (int) p->parts.size();
    g.setFont (font (13.0f));
    g.setColour (col::dim);
    g.drawText (vault::categoryNames()[jlimit (0, (int) vault::numCategories - 1, p->category)] + midDot() + String (n) + " " + layerWord (n),
                catR, Justification::centredLeft, true);

    // tag pills (styles then characters), wrapping onto at most 2 rows
    {
        auto tags = vault::namesOfMask (p->styles, vault::styleNames());
        tags.addArray (vault::namesOfMask (p->chars, vault::characterNames()));
        const auto f = font (12.5f);
        float x = (float) tagsR.getX(), y = (float) tagsR.getY();
        const float h = 22.0f;
        for (int i = 0; i < tags.size(); ++i)
        {
            const float w = textWidth (f, tags[i]) + 20.0f;
            if (x + w > (float) tagsR.getRight() && x > (float) tagsR.getX()) { x = (float) tagsR.getX(); y += h + 6.0f; }
            if (y + h > (float) tagsR.getBottom() + 0.5f) break;
            const Rectangle<float> pill (x, y, w, h);
            g.setColour (col::fieldLine);
            g.drawRoundedRectangle (pill.reduced (0.5f), h * 0.5f, 1.0f);
            g.setColour (col::muted);
            g.setFont (f);
            g.drawText (tags[i], pill, Justification::centred, false);
            x += w + 6.0f;
        }
    }

    // waveform / level thumbnail (rendered in the background; flat line until it is ready)
    {
        auto w = waveR.toFloat();
        g.setColour (col::field);
        g.fillRoundedRectangle (w, 12.0f);
        g.setColour (col::layerLine);
        g.drawRoundedRectangle (w.reduced (0.5f), 12.0f, 1.0f);
        VaultThumbs::Thumb t;
        auto inner = w.reduced (10.0f, 6.0f);
        float macros[vault::numMacros];
        for (int i = 0; i < vault::numMacros; ++i) macros[i] = proc.getMacro (i);
        if (thumbs.getLive (preset, macros, t))   // follows the 4 macro knobs
        {
            g.setGradientFill (ColourGradient (col::deep, inner.getX(), 0, col::ice, inner.getRight(), 0, false));
            const float step = inner.getWidth() / (float) VaultThumbs::numBins;
            for (int i = 0; i < VaultThumbs::numBins; ++i)
            {
                const float hgt = std::max (2.0f, t.level[(size_t) i] * inner.getHeight());
                g.fillRoundedRectangle (inner.getX() + (float) i * step + step * 0.2f, inner.getCentreY() - hgt * 0.5f, step * 0.6f, hgt, 1.0f);
            }
        }
        else
        {
            g.setColour (col::fieldLine);
            g.fillRect (inner.getX(), inner.getCentreY() - 1.0f, inner.getWidth(), 2.0f);
        }
    }

    // layers
    g.setFont (font (11.5f, true, 0.16f));
    g.setColour (col::ice);
    g.drawText ("LAYERS" + midDot() + String (n), layersLabelR, Justification::centredLeft, false);
    {
        auto s = stackR;
        for (int i = 0; i < vault::maxParts; ++i)
        {
            auto row = s.removeFromTop (30).toFloat();
            s.removeFromTop (6);
            if (i >= n)
            {
                g.setColour (col::layerLine.withAlpha (0.45f));
                g.drawRoundedRectangle (row.reduced (0.5f), 10.0f, 1.0f);
                continue;
            }
            const auto& part = p->parts[(size_t) i];
            g.setColour (col::layerBg);
            g.fillRoundedRectangle (row, 10.0f);
            g.setColour (col::layerLine);
            g.drawRoundedRectangle (row.reduced (0.5f), 10.0f, 1.0f);
            auto t = row.reduced (10.0f, 0.0f);
            g.setFont (font (11.5f, true));
            g.setColour (col::ice);
            g.drawText (String::charToString ((juce_wchar) ('A' + i)), t.removeFromLeft (20.0f), Justification::centredLeft, false);
            String right = String (roundToInt (part.level * 100.0f)) + " %";
            const int tr = part.octave * 12 + part.semi;
            if (tr != 0) right = (tr % 12 == 0 ? (tr > 0 ? "+" : "") + String (tr / 12) + " oct" : (tr > 0 ? "+" : "") + String (tr) + " st") + "   " + right;
            g.setFont (font (12.5f));
            g.setColour (col::dim);
            const float rw = textWidth (font (12.5f), right) + 2.0f;
            g.drawText (right, t.removeFromRight (rw), Justification::centredRight, false);
            g.setFont (font (13.0f));
            g.setColour (col::text);
            g.drawText (part.label.isNotEmpty() ? part.label : "Layer " + String::charToString ((juce_wchar) ('A' + i)),
                        t.withTrimmedRight (8.0f), Justification::centredLeft, true);
        }
    }

    // volume
    g.setFont (font (11.5f, true, 0.12f));
    g.setColour (col::dim);
    g.drawText ("VOLUME", volumeR.withWidth (66), Justification::centredLeft, false);
    const float db = proc.getVolumeDb();
    g.setFont (font (12.5f));
    g.setColour (volume.isMouseOverOrDragging() ? col::ice : col::muted);
    g.drawText (db <= -59.9f ? String ("-inf dB") : String (db, 1) + " dB", volumeR.withLeft (volumeR.getRight() - 58), Justification::centredRight, false);
}

void VaultInfoPanel::mouseMove (const juce::MouseEvent& e)
{
    const bool o = heartRect().contains (e.position);
    if (o != heartOver) { heartOver = o; repaint (heartRect().toNearestInt().expanded (2)); }
    setMouseCursor (o ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void VaultInfoPanel::mouseExit (const juce::MouseEvent&) { if (heartOver) { heartOver = false; repaint(); } }
void VaultInfoPanel::mouseDown (const juce::MouseEvent& e)
{
    if (heartRect().contains (e.position) && preset >= 0 && onToggleFavorite) onToggleFavorite (preset);
}

//==============================================================================
// VaultMainView
//==============================================================================
VaultMainView::VaultMainView (VaultProcessor& p, std::shared_ptr<VaultSettings> s)
    : proc (p), settings (std::move (s)), model (p.bank()), thumbs (p.bank()),
      categories (model), chips (model.filter), list (p.bank(), model), info (p, model, thumbs), keyboard (p.getKeyboardState())
{
    model.setFavoriteIds (settings->favorites());
    model.setRecentIds (settings->recent());
    model.refilter();

    search.setFont (font (14.5f));
    search.setTextToShowWhenEmpty ("Search: dark bell, glide 808", col::dim);
    search.setIndents (0, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.setSelectAllWhenFocused (false);
    search.setEscapeAndReturnKeysConsumed (true);
    search.setTitle ("Search sounds");
    search.onTextChange = [this] { model.filter.query = search.getText(); filterChanged(); };
    search.onEscapeKey = [this]
    {
        if (search.isEmpty()) { giveFocusBack(); return; }
        search.clear();
        model.filter.query.clear();
        filterChanged();
    };
    search.onReturnKey = [this] { if (model.rowOf (shown) < 0 && visibleCount() > 0) loadPreset (model.visible().front(), true); giveFocusBack(); };
    search.addKeyListener (&searchKeys);
    search.onFocusLost = [this] { repaint (searchRect().expanded (2)); };
    addAndMakeVisible (search);

    prevBtn.onClick = [this] { step (-1, true); };
    nextBtn.onClick = [this] { step (1, true); };
    randomBtn.onClick = [this] { randomSound(); };
    recentBtn.onClick = [this] { setRecentOnly (! model.filter.recentOnly); };
    favBtn.onClick = [this] { setFavoritesOnly (! model.filter.favoritesOnly); };
    for (auto* b : std::initializer_list<juce::Component*> { &prevBtn, &nextBtn, &randomBtn, &recentBtn, &favBtn }) addAndMakeVisible (b);

    categories.onSelect = [this] (int c) { setCategory (c); };
    addAndMakeVisible (categories);
    chips.onChange = [this] { filterChanged(); };
    addAndMakeVisible (chips);
    list.onClick = [this] (int idx) { loadPreset (idx, true); };
    list.onToggleFavorite = [this] (int idx) { toggleFavorite (idx); };
    addAndMakeVisible (list);
    info.onToggleFavorite = [this] (int idx) { toggleFavorite (idx); };
    info.dragPill.onDragOut = [this]
    {
        const auto f = renderWavForDrag();
        if (f.existsAsFile())
        {
            showStatus ("Drop the WAV into your DAW or a folder: " + f.getFileName());
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, &info.dragPill);
        }
        else showStatus ("Could not write the WAV file.");
    };
    info.dragPill.onClickOnly = [this] { showStatus ("Drag this button into your DAW or a folder to drop the sound in as a 2 s WAV."); };
    addAndMakeVisible (info);

    previewBtn.setClickingTogglesState (false);
    previewBtn.onClick = [this] { settings->setPreviewOnClick (! settings->previewOnClick()); updateToggles(); };
    scaleBtn.onClick = [this] { showScaleMenu(); };
    addAndMakeVisible (previewBtn);
    addAndMakeVisible (scaleBtn);
    addAndMakeVisible (keyboard);

    thumbs.onReady = [this] (int idx) { if (idx == shown) info.repaint(); };
    settings->addChangeListener (this);
    noClickFocus (*this, &search);
    setWantsKeyboardFocus (false);

    updateToggles();
    const int cur = proc.currentPreset();
    setShown (cur);
    list.showRow (model.rowOf (cur));
}

VaultMainView::~VaultMainView()
{
    search.removeKeyListener (&searchKeys);
    settings->removeChangeListener (this);
    thumbs.onReady = nullptr;
}

//==============================================================================
int VaultMainView::logoWidth()
{
    return (int) std::ceil (textWidth (font (22.0f, true, 0.14f), "CHROME VAULT"));
}
juce::Rectangle<int> VaultMainView::searchRect() const
{
    const int left = 20 + logoWidth() + 22, curW = 268;
    const int right = randomBtn.getX() - 12 - curW - 14;
    return { left, 14, right - left, 38 };
}
juce::Rectangle<int> VaultMainView::curRect() const { return { randomBtn.getX() - 12 - 268, 15, 268, 36 }; }
juce::Rectangle<int> VaultMainView::statusRect() const { return { 0, getHeight() - keysH - statusH, getWidth(), statusH }; }
juce::Rectangle<int> VaultMainView::keysRect() const { return { 0, getHeight() - keysH, getWidth(), keysH }; }

void VaultMainView::resized()
{
    // top bar, right to left
    {
        int x = getWidth() - 20;
        auto place = [&] (PillButton& b) { const int w = b.idealWidth(); x -= w; b.setBounds (x, 16, w, 34); x -= 8; };
        place (favBtn); place (recentBtn); place (randomBtn);
        const auto c = curRect();
        prevBtn.setBounds (c.getX() + 5, c.getY() + 4, 28, 28);
        nextBtn.setBounds (c.getRight() - 33, c.getY() + 4, 28, 28);
        search.setBounds (searchRect().withTrimmedLeft (40).withTrimmedRight (14).reduced (0, 6));
    }
    auto main = getLocalBounds().withTrimmedTop (topH).withTrimmedBottom (statusH + keysH);
    categories.setBounds (main.removeFromLeft (catW));
    info.setBounds (main.removeFromRight (infoW));
    chips.setBounds (main.removeFromTop (chips.idealHeight()));
    list.setBounds (main.withTrimmedTop (1));

    auto st = statusRect().reduced (20, 5);
    const int sw = scaleBtn.idealWidth();
    scaleBtn.setBounds (st.removeFromRight (sw));
    st.removeFromRight (8);
    const int pw = previewBtn.idealWidth() + 12;
    previewBtn.setBounds (st.removeFromRight (pw));
    keyboard.setBounds (keysRect().withTrimmedLeft (20).withTrimmedRight (20).withTrimmedBottom (16));
}

void VaultMainView::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (col::plugA, 0, 0, col::plugB, 0, b.getBottom(), false));
    g.fillRect (b);

    // top bar
    g.setGradientFill (ColourGradient (col::topA, 0, 0, col::topB, 0, (float) topH, false));
    g.fillRect (0.0f, 0.0f, b.getWidth(), (float) topH);
    g.setColour (col::line);
    g.fillRect (0.0f, (float) topH - 1.0f, b.getWidth(), 1.0f);
    {   // logo
        g.setGradientFill (chromeGradient (13.0f, 38.0f));
        g.setFont (font (22.0f, true, 0.14f));
        g.drawText ("CHROME VAULT", 20, 12, logoWidth() + 10, 28, Justification::centredLeft, false);
        g.setColour (col::dim);
        g.setFont (font (10.5f, true, 0.3f));
        g.drawText ("BY TRAPWIN", 21, 38, 200, 14, Justification::centredLeft, false);
    }
    {   // search field
        const auto r = searchRect().toFloat();
        g.setColour (col::field);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (search.hasKeyboardFocus (true) ? col::ice2 : col::fieldLine);
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        drawSearchIcon (g, { r.getX() + 16.0f, r.getCentreY() - 8.0f, 16.0f, 16.0f }, col::dim);
    }
    {   // current preset pill
        const auto r = curRect().toFloat();
        g.setColour (col::field);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (col::fieldLine);
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        const auto* p = proc.currentPresetData();
        g.setColour (col::silver);
        g.setFont (font (13.5f, true));
        g.drawText (p != nullptr ? p->name : String ("No sounds"), r.reduced (38.0f, 0.0f), Justification::centred, true);
    }

    // column borders
    const float mainTop = (float) topH, mainBottom = (float) (getHeight() - statusH - keysH);
    g.setColour (col::line);
    g.fillRect ((float) catW - 1.0f, mainTop, 1.0f, mainBottom - mainTop);
    g.fillRect ((float) catW, (float) chips.getBottom(), (float) (getWidth() - catW - infoW), 1.0f);

    // status bar
    const auto st = statusRect();
    g.fillRect ((float) st.getX(), (float) st.getY(), (float) st.getWidth(), 1.0f);
    {
        float x = 20.0f;
        const float y = (float) st.getY(), h = (float) st.getHeight();
        auto put = [&] (const String& s, bool strong)
        {
            const auto f = font (13.0f, strong);
            g.setFont (f);
            g.setColour (strong ? col::ice : col::dim);
            const float w = textWidth (f, s);
            g.drawText (s, Rectangle<float> (x, y, w + 2.0f, h), Justification::centredLeft, false);
            x += w;
        };
        put ("Showing ", false);
        put (thousands (visibleCount()), true);
        put (" of ", false);
        put (thousands (model.size()), true);
        if (statusMsg.isNotEmpty() && Time::getMillisecondCounter() < statusUntil)
            put (midDot() + statusMsg, false);
        else
            put (midDot() + String (CharPointer_UTF8 ("\xe2\x86\x91 \xe2\x86\x93 or \xe2\x97\x80 \xe2\x96\xb6 next sound")) + midDot() + "Instant load"
                     + midDot() + "Keys A-L play, Z / X octave", false);
    }

    // keyboard bed
    const auto kr = keysRect();
    g.setColour (col::field);
    g.fillRect (kr);
}

//==============================================================================
void VaultMainView::setShown (int idx)
{
    shown = idx;
    list.setCurrent (idx);
    info.setPreset (idx);
    repaint (curRect());
    if (idx >= 0)
    {
        VaultThumbs::Thumb t;
        thumbs.get (idx, t);
        for (int d : { 1, -1, 2 }) thumbs.prefetch (model.step (idx, d, true));
    }
}

void VaultMainView::loadPreset (int idx, bool audition)
{
    if (! juce::isPositiveAndBelow (idx, model.size())) return;
    proc.selectPreset (idx);
    setShown (idx);
    model.pushRecent (idx);
    settings->setRecent (model.recentIds());
    list.showRow (model.rowOf (idx));
    if (audition && settings->previewOnClick()) auditionCurrent();
}

void VaultMainView::auditionCurrent()
{
    if (const auto* p = proc.currentPresetData())
        proc.previewNote (p->category == vault::cat808Bass ? 48 : 60, 0.8f, p->category == vault::catPads || p->category == vault::catFxAtmos ? 1.4 : 0.9);
}

void VaultMainView::step (int delta, bool wrap)
{
    const int next = model.step (shown, delta, wrap);
    if (next >= 0 && next != shown) loadPreset (next, true);
}

int VaultMainView::pageRows() const
{
    return std::max (1, list.getHeight() / vui::PresetList::rowH - 1);
}

void VaultMainView::stepToEnd (bool last)
{
    const auto& v = model.visible();
    if (v.empty()) return;
    const int target = last ? v.back() : v.front();
    if (target != shown) loadPreset (target, true);
}

void VaultMainView::randomSound()
{
    const auto& v = model.visible();
    if (v.empty()) return;
    int pick = v[(size_t) rng.nextInt ((int) v.size())];
    if (pick == shown && v.size() > 1) pick = v[(size_t) ((model.rowOf (pick) + 1 + rng.nextInt ((int) v.size() - 1)) % (int) v.size())];
    loadPreset (pick, true);
}

void VaultMainView::syncFromProcessor()
{
    const int cur = proc.currentPreset();
    if (cur != shown) { setShown (cur); list.showRow (model.rowOf (cur)); }
    if (statusUntil != 0 && juce::Time::getMillisecondCounter() >= statusUntil) { statusUntil = 0; repaint (statusRect()); }
}

void VaultMainView::filterChanged (bool scrollToCurrent, bool keepScroll)
{
    model.refilter();
    list.refresh();
    const int row = model.rowOf (shown);
    if (scrollToCurrent && row >= 0) list.showRow (row);
    else if (! keepScroll) list.scrollToTop();
    categories.setSelected (model.filter.category);
    chips.repaint();
    updateToggles();
    repaint (statusRect());
}

void VaultMainView::setCategory (int c) { model.filter.category = juce::jlimit (-1, (int) vault::numCategories - 1, c); filterChanged(); }
void VaultMainView::setQuery (const juce::String& q) { search.setText (q, false); model.filter.query = q; filterChanged(); }
void VaultMainView::setStyle (int s) { model.filter.style = juce::jlimit (-1, (int) vault::numStyles - 1, s); filterChanged(); }
void VaultMainView::setCharacter (int c) { model.filter.character = juce::jlimit (-1, (int) vault::numCharacters - 1, c); filterChanged(); }
void VaultMainView::setLayers (int n) { model.filter.layers = juce::jlimit (0, (int) vault::maxParts, n); filterChanged(); }
void VaultMainView::setFavoritesOnly (bool on) { model.filter.favoritesOnly = on; filterChanged(); }
void VaultMainView::setRecentOnly (bool on) { model.filter.recentOnly = on; filterChanged (! on); }

void VaultMainView::toggleFavorite (int idx)
{
    model.setFavorite (idx, ! model.isFavorite (idx));
    settings->setFavorites (model.favoriteIds());
    if (model.filter.favoritesOnly) filterChanged (false, true);   // the row goes away, the list stays where it is
    list.refresh();
    info.repaint();
}

void VaultMainView::giveFocusBack()
{
    if (auto* ed = findParentComponentOfClass<juce::AudioProcessorEditor>(); ed != nullptr && ed->isShowing()) ed->grabKeyboardFocus();
    else unfocusAllComponents();
}

void VaultMainView::updateToggles()
{
    recentBtn.setToggleState (model.filter.recentOnly, juce::dontSendNotification);
    favBtn.setToggleState (model.filter.favoritesOnly, juce::dontSendNotification);
    previewBtn.setToggleState (settings->previewOnClick(), juce::dontSendNotification);
    const float s = currentScale ? currentScale() : 1.0f;
    const auto t = juce::String (juce::roundToInt (s * 100.0f)) + " %";
    if (scaleBtn.getButtonText() != t)
    {
        scaleBtn.setButtonText (t);
        if (getWidth() > 0) resized();
    }
}

void VaultMainView::showStatus (const juce::String& msg)
{
    statusMsg = msg;
    statusUntil = juce::Time::getMillisecondCounter() + 4000;
    repaint (statusRect());
}

void VaultMainView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // another plugin window changed favorites / preview: follow it
    auto fresh = settings->favorites();
    auto mine = model.favoriteIds();
    fresh.sort (false); mine.sort (false);
    if (fresh != mine)
    {
        model.setFavoriteIds (fresh);
        if (model.filter.favoritesOnly) filterChanged (false, true);
        list.refresh();
        info.repaint();
    }
    updateToggles();
}

void VaultMainView::showScaleMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    const float cur = currentScale ? currentScale() : 1.0f;
    int id = 1;
    for (float s : VaultEditor::scalePresets())
        m.addItem (id++, juce::String (juce::roundToInt (s * 100.0f)) + " %", true, std::abs (s - cur) < 0.005f);
    juce::Component::SafePointer<VaultMainView> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&scaleBtn).withMinimumWidth (scaleBtn.getWidth()),
                     [safe] (int r)
                     {
                         if (safe == nullptr || r <= 0) return;
                         if (safe->onScaleChosen) safe->onScaleChosen (VaultEditor::scalePresets()[(size_t) r - 1]);
                         safe->updateToggles();
                     });
}

juce::File VaultMainView::renderWavForDrag()
{
    const auto* p = proc.currentPresetData();
    if (p == nullptr) return {};
    float macros[vault::numMacros];
    for (int i = 0; i < vault::numMacros; ++i) macros[i] = proc.getMacro (i);
    constexpr double sr = 44100.0;
    const bool low = p->category == vault::cat808Bass;   // same note as the preview: 808s / basses at C3, the rest at C4
    const auto buf = VaultEngine::render (*p, low ? 48 : 60, 0.8f, 2.0, sr, macros);
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("CHROME VAULT");
    dir.createDirectory();
    const auto f = dir.getChildFile (juce::File::createLegalFileName (p->name + (low ? " C3" : " C4")) + ".wav");
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    if (os == nullptr) return {};
    auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (24));
    if (w == nullptr || ! w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples())) return {};
    w.reset();
    return f;
}

//==============================================================================
// VaultEditor
//==============================================================================
const std::vector<float>& VaultEditor::scalePresets()
{
    static const std::vector<float> s { 0.7f, 0.8f, 0.9f, 1.0f, 1.15f, 1.25f, 1.5f, 1.75f };
    return s;
}

float VaultEditor::firstOpenScale (juce::Rectangle<int> usable)
{
    if (usable.isEmpty()) return 1.0f;
    const auto area = usable.toFloat() * 0.85f;
    float s = minScale;
    for (float c : scalePresets())
        if (c <= 1.0f && (float) baseWidth * c <= area.getWidth() && (float) baseHeight * c <= area.getHeight()) s = c;
    return s;
}

namespace
{
const char pcKeys[] = "awsedftgyhujkolp;";
}

VaultEditor::VaultEditor (VaultProcessor& p) : AudioProcessorEditor (p), proc (p), settings (VaultSettings::get())
{
    pcNoteDown.fill (-1);
    setLookAndFeel (&lnf);
    main = std::make_unique<VaultMainView> (proc, settings);
    main->currentScale = [this] { return uiScale; };
    main->onScaleChosen = [this] (float s) { setUiScale (s); };
    addAndMakeVisible (*main);

    // first open: the largest preset up to 100 % that fits 85 % of the screen the plugin opens on (the one under the mouse -
    // the user just clicked the plugin in the DAW there); later: the remembered scale
    float s = settings->scale();
    if (s <= 0.0f)
    {
        const auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* d = displays.getDisplayForPoint (juce::Desktop::getMousePosition());
        if (d == nullptr) d = displays.getPrimaryDisplay();
        s = d != nullptr ? firstOpenScale (d->userArea) : 1.0f;
    }
    s = juce::jlimit (minScale, maxScale, s);

    setWantsKeyboardFocus (true);
    addMouseListener (this, true);
    settingSize = true;   // the limits below resize the window once - don't remember that size
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (baseWidth * minScale), juce::roundToInt (baseHeight * minScale),
                     juce::roundToInt (baseWidth * maxScale), juce::roundToInt (baseHeight * maxScale));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (juce::roundToInt (baseWidth * s), juce::roundToInt (baseHeight * s));
    settingSize = false;
    startTimerHz (30);
}

VaultEditor::~VaultEditor()
{
    stopTimer();
    removeMouseListener (this);
    releasePcNotes();
    main.reset();
    settings->flush();
    setLookAndFeel (nullptr);
}

void VaultEditor::setUiScale (float s)
{
    s = juce::jlimit (minScale, maxScale, s);
    setSize (juce::roundToInt (baseWidth * s), juce::roundToInt (baseHeight * s));
}

void VaultEditor::paint (juce::Graphics& g) { g.fillAll (vui::col::ink); }

void VaultEditor::resized()
{
    if (main == nullptr) return;
    // the scale that fits both sides (a host may ignore the aspect ratio), the view centred in any leftover space
    const float fit = std::min ((float) getWidth() / (float) baseWidth, (float) getHeight() / (float) baseHeight);
    uiScale = juce::jlimit (minScale, maxScale, fit);
    const float dx = std::max (0.0f, ((float) getWidth() - (float) baseWidth * uiScale) * 0.5f);
    const float dy = std::max (0.0f, ((float) getHeight() - (float) baseHeight * uiScale) * 0.5f);
    main->setBounds (0, 0, baseWidth, baseHeight);
    main->setTransform (juce::AffineTransform::scale (uiScale).translated (std::round (dx), std::round (dy)));
    if (! settingSize) settings->setScale (uiScale);
    main->updateToggles();
    main->repaint();
}

void VaultEditor::visibilityChanged()
{
    focusGrabbed = false;
}

void VaultEditor::timerCallback()
{
    if (main == nullptr) return;
    main->syncFromProcessor();
    if (! focusGrabbed && isShowing())
    {
        focusGrabbed = true;
        if (! main->searchHasFocus()) grabKeyboardFocus();
    }
}

void VaultEditor::mouseDown (const juce::MouseEvent& e)
{
    // a click anywhere but in the search box gives the keyboard back to the sounds (arrows, PC keys)
    if (main != nullptr && e.eventComponent != nullptr && ! main->searchBox().isParentOf (e.eventComponent) && e.eventComponent != &main->searchBox())
        if (! hasKeyboardFocus (false)) grabKeyboardFocus();
}

bool VaultEditor::keyPressed (const juce::KeyPress& k)
{
    if (main == nullptr || main->searchHasFocus()) return false;
    const auto mods = k.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown()) return false;
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::downKey)  { main->step (1, false); return true; }
    if (code == juce::KeyPress::upKey)    { main->step (-1, false); return true; }
    if (code == juce::KeyPress::rightKey) { main->step (1, true); return true; }
    if (code == juce::KeyPress::leftKey)  { main->step (-1, true); return true; }
    if (code == juce::KeyPress::pageDownKey) { main->step (main->pageRows(), false); return true; }
    if (code == juce::KeyPress::pageUpKey)   { main->step (-main->pageRows(), false); return true; }
    if (code == juce::KeyPress::homeKey)     { main->stepToEnd (false); return true; }
    if (code == juce::KeyPress::endKey)      { main->stepToEnd (true); return true; }
    if (code == juce::KeyPress::returnKey) { main->auditionCurrent(); return true; }
    // Space is NOT used: it stays with the DAW (play / stop)
    const auto ch = juce::CharacterFunctions::toLowerCase (k.getTextCharacter());
    if (ch == 'z' || ch == 'x')
    {
        releasePcNotes();
        pcOctaveBase = juce::jlimit (24, 96, pcOctaveBase + (ch == 'x' ? 12 : -12));
        main->keys().pcBase = pcOctaveBase;
        main->keys().repaint();
        main->showStatus ("PC keys: A plays C" + juce::String (pcOctaveBase / 12 - 1));
        return true;
    }
    for (const char* c = pcKeys; *c != 0; ++c) if (ch == (juce::juce_wchar) *c) return true;   // played in keyStateChanged
    return false;
}

bool VaultEditor::keyStateChanged (bool)
{
    if (main == nullptr || main->searchHasFocus()) return false;
    bool used = false;
    auto& ks = proc.getKeyboardState();
    for (int i = 0; pcKeys[i] != 0; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown ((int) pcKeys[i]);
        if (down && pcNoteDown[(size_t) i] < 0)
        {
            const int note = juce::jlimit (0, 127, pcOctaveBase + i);
            pcNoteDown[(size_t) i] = note;
            ks.noteOn (1, note, 0.8f);
            used = true;
        }
        else if (! down && pcNoteDown[(size_t) i] >= 0)
        {
            ks.noteOff (1, pcNoteDown[(size_t) i], 0.0f);
            pcNoteDown[(size_t) i] = -1;
            used = true;
        }
    }
    return used;
}

void VaultEditor::focusLost (FocusChangeType) { releasePcNotes(); }

void VaultEditor::releasePcNotes()
{
    for (auto& n : pcNoteDown)
        if (n >= 0) { proc.getKeyboardState().noteOff (1, n, 0.0f); n = -1; }
}
