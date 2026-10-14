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
    catR = r.removeFromTop (20);
    r.removeFromTop (8);
    tagsR = r.removeFromTop (54);
    r.removeFromTop (12);
    waveR = r.removeFromTop (58);
    r.removeFromTop (14);
    layersLabelR = r.removeFromTop (26);
    r.removeFromTop (8);
    stackR = r.removeFromTop (4 * 30 + 3 * 6);

    volumeR = r.removeFromBottom (26);
    r.removeFromBottom (12);
    knobsR = r.removeFromBottom (84);
    const int kw = knobsR.getWidth() / vault::numMacros;
    for (int i = 0; i < vault::numMacros; ++i) knob[i].setBounds (knobsR.getX() + i * kw, knobsR.getY(), kw, knobsR.getHeight());
    auto v = volumeR;
    v.removeFromLeft (volumeLabelW);
    v.removeFromRight (volumeValueW + 6);
    volume.setBounds (v);
    dragPill.setBounds (layersLabelR.withLeft (layersLabelR.getRight() - 118));
}

juce::Rectangle<float> VaultInfoPanel::heartRect() const
{
    return juce::Rectangle<float> (26.0f, 26.0f).withCentre ({ (float) nameR.getRight() - 11.0f, (float) nameR.getCentreY() });
}

VaultInfoPanel::Notice VaultInfoPanel::notice() const
{
    if (preset < 0 || preset != proc.currentPreset()) return Notice::none;
    if (proc.isMissingSamples()) return proc.libraryFound() ? Notice::instrumentMissing : Notice::libraryMissing;
    if (proc.isLoading()) return Notice::loading;
    return Notice::none;
}

VaultInfoPanel::PartState VaultInfoPanel::partState (const VaultPart& part) const
{
    if (part.kind != VaultPartKind::sample) return PartState::ready;
    const auto* lib = proc.sampleLibrary();
    const auto* h = lib != nullptr && lib->libraryFound() ? lib->find (part.source) : nullptr;
    if (h == nullptr || h->hasFailed()) return PartState::missing;
    // only the shown sound asks for its samples; another one loads when it is picked
    return ! h->isLoaded() && preset == proc.currentPreset() ? PartState::loading : PartState::ready;
}

juce::String VaultInfoPanel::noticeTitle() const
{
    switch (notice())
    {
        case Notice::libraryMissing:    return juce::String (juce::CharPointer_UTF8 ("Sound library not found \xe2\x80\x94 run the CHROME VAULT installer"));
        case Notice::instrumentMissing: return juce::String (juce::CharPointer_UTF8 ("This sound needs a newer sound library \xe2\x80\x94 run the latest CHROME VAULT installer"));
        case Notice::none: case Notice::loading: default: return {};
    }
}

void VaultInfoPanel::repaintStatus() { repaint (tagsR.getUnion (stackR).expanded (2)); }

void VaultInfoPanel::paintNotice (juce::Graphics& g, juce::Rectangle<float> r, Notice) const
{
    using namespace juce;
    g.setColour (col::field);
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (col::layerLine);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    // a slim accent on the left: noticeable, not alarming
    g.setColour (col::ice2.withAlpha (0.7f));
    g.fillRoundedRectangle (r.getX() + 7.0f, r.getY() + 12.0f, 2.5f, r.getHeight() - 24.0f, 1.25f);

    auto t = r.reduced (16.0f, 8.0f).withTrimmedLeft (4.0f);
    const float lineH = 18.0f, gap = 4.0f;
    const auto fTitle = font (14.0f, true), fBody = font (14.0f);
    bool anySynth = false;
    if (isPositiveAndBelow (preset, (int) proc.bank().size()))
        for (auto& part : proc.bank()[(size_t) preset].parts) anySynth |= part.kind != VaultPartKind::sample;
    const auto title = wrapText (fTitle, noticeTitle(), t.getWidth());
    const auto body = wrapText (fBody, anySynth ? "Its synth layers still play." : "Synth sounds keep playing.", t.getWidth());
    const auto path = wrapText (fBody, proc.libraryPath().getFullPathName(), t.getWidth());
    float total = (float) (title.size() + body.size() + path.size()) * lineH + gap;
    const bool withBody = total <= t.getHeight();
    if (! withBody) total -= (float) body.size() * lineH;
    float y = t.getY() + std::max (0.0f, (t.getHeight() - total) * 0.5f);
    auto put = [&] (const StringArray& lines, const Font& f, Colour c)
    {
        g.setFont (f);
        g.setColour (c);
        for (auto& l : lines)
        {
            if (y + lineH > t.getBottom() + 0.5f) return;
            g.drawText (l, Rectangle<float> (t.getX(), y, t.getWidth() + 2.0f, lineH), Justification::centredLeft, false);
            y += lineH;
        }
    };
    put (title, fTitle, col::text);
    if (withBody) put (body, fBody, col::muted);
    y += gap;
    put (path, fBody, col::dim);
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
        g.setFont (font (15.0f));
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
    g.setFont (font (14.0f));
    g.setColour (col::dim);
    g.drawText (vault::categoryNames()[jlimit (0, (int) vault::numCategories - 1, p->category)] + midDot() + String (n) + " " + layerWord (n),
                catR, Justification::centredLeft, true);

    const auto state = notice();
    if (state == Notice::libraryMissing || state == Notice::instrumentMissing)
    {
        // a calm card instead of the pills and the (silent) waveform: what is missing, what to do, where it was looked for
        paintNotice (g, tagsR.getUnion (waveR).toFloat(), state);
    }
    else
    {
    // pills: what plays the layers (sampled / modelled / synth), the loading state and the home note
    {
        StringArray tags;
        bool sampled = false, synth = false, modelled = false;
        for (auto& part : p->parts)
        {
            sampled |= part.kind == VaultPartKind::sample;
            synth |= part.kind == VaultPartKind::synth;
            modelled |= part.kind == VaultPartKind::model;
        }
        if (sampled) tags.add ("Sampled");
        if (modelled) tags.add ("Modelled");
        if (synth) tags.add ("Synth");
        const String loadingText (CharPointer_UTF8 ("Loading\xe2\x80\xa6"));
        if (state == Notice::loading) tags.add (loadingText);
        tags.add ("Plays at " + vault::noteName (vault::homeNote));
        const auto f = font (14.0f);
        float x = (float) tagsR.getX(), y = (float) tagsR.getY();
        const float h = 24.0f, spin = 14.0f;
        for (int i = 0; i < tags.size(); ++i)
        {
            const bool loading = tags[i] == loadingText;
            const float w = textWidth (f, tags[i]) + 18.0f + (loading ? spin + 6.0f : 0.0f);
            if (x + w > (float) tagsR.getRight() && x > (float) tagsR.getX()) { x = (float) tagsR.getX(); y += h + 6.0f; }
            if (y + h > (float) tagsR.getBottom() + 0.5f) break;
            Rectangle<float> pill (x, y, w, h);
            g.setColour (loading ? col::ice2.withAlpha (0.6f) : col::fieldLine);
            g.drawRoundedRectangle (pill.reduced (0.5f), h * 0.5f, 1.0f);
            auto t = pill;
            if (loading)
            {
                t.removeFromLeft (9.0f);
                drawSpinner (g, t.removeFromLeft (spin).withSizeKeepingCentre (spin, spin), col::ice2);
                t.removeFromRight (3.0f);
            }
            g.setColour (col::muted);
            g.setFont (f);
            g.drawText (tags[i], t, Justification::centred, false);
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
    }

    // layers: letter, what plays it (icon + instrument / model / synth layer name, the label when it adds something),
    // transpose and level - or the loading / missing state of a sampled layer
    g.setFont (font (14.0f, true, 0.08f));
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
            g.setFont (font (14.0f, true));
            g.setColour (col::ice);
            g.drawText (String::charToString ((juce_wchar) ('A' + i)), t.removeFromLeft (18.0f), Justification::centredLeft, false);
            const auto st = partState (part);
            auto icon = t.removeFromLeft (16.0f).withSizeKeepingCentre (16.0f, 12.0f);
            if (st == PartState::loading) drawSpinner (g, icon.withSizeKeepingCentre (14.0f, 14.0f), col::ice2);
            else drawKindIcon (g, icon, part.kind, st == PartState::missing ? col::heartOff : col::muted);
            t.removeFromLeft (8.0f);
            String right = String (roundToInt (part.level * 100.0f)) + " %";
            const int tr = part.octave * 12 + part.semi;
            if (tr != 0) right = (tr % 12 == 0 ? (tr > 0 ? "+" : "") + String (tr / 12) + " oct" : (tr > 0 ? "+" : "") + String (tr) + " st") + "   " + right;
            if (st == PartState::missing) right = "not installed";
            const auto f = font (14.0f);
            g.setFont (f);
            g.setColour (col::dim);
            const float rw = textWidth (f, right) + 2.0f;
            g.drawText (right, t.removeFromRight (rw), Justification::centredLeft, false);
            t.removeFromRight (8.0f);
            // the instrument (always whole when it fits), then the layer's own label in the dim colour if it says more
            const auto inst = VaultBrowserModel::instrumentName (part, proc.sampleLibrary());
            const float iw = std::min (t.getWidth(), textWidth (f, inst) + 1.0f);
            g.setColour (st == PartState::missing ? col::dim : col::text);
            g.drawText (inst, t.removeFromLeft (iw), Justification::centredLeft, true);
            const auto label = part.label.trim();
            if (label.isNotEmpty() && ! inst.containsIgnoreCase (label) && ! label.containsIgnoreCase (inst))
            {
                const auto extra = midDot() + label;
                if (textWidth (f, extra) <= t.getWidth())
                {
                    g.setColour (col::dim);
                    g.drawText (extra, t, Justification::centredLeft, false);
                }
            }
        }
    }

    // volume
    g.setFont (font (14.0f, true, 0.04f));
    g.setColour (col::dim);
    g.drawText ("VOLUME", volumeR.withWidth (volumeLabelW), Justification::centredLeft, false);
    const float db = proc.getVolumeDb();
    g.setFont (font (14.0f));
    g.setColour (volume.isMouseOverOrDragging() ? col::ice : col::muted);
    g.drawText (db <= -59.9f ? String ("-inf dB") : String (db, 1) + " dB", volumeR.withLeft (volumeR.getRight() - volumeValueW), Justification::centredRight, false);
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
    : proc (p), settings (std::move (s)), model (p.bank(), p.sampleLibrary()), thumbs (p.bank(), p.sampleLibrary()),
      categories (model), chips (model.filter), list (p.bank(), model), info (p, model, thumbs), keyboard (p.getKeyboardState())
{
    model.setFavoriteIds (settings->favorites());
    model.setRecentIds (settings->recent());
    model.refilter();

    search.setFont (font (15.0f));
    search.setTextToShowWhenEmpty ("Search: piano, cello, choir", col::dim);
    search.setIndents (0, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.setSelectAllWhenFocused (false);
    search.setEscapeAndReturnKeysConsumed (true);
    search.setTitle ("Search sounds");
    search.onTextChange = [this] { model.filter.query = search.getText(); filterChanged(); };
    search.onEscapeKey = [this]
    {
        if (! search.isEmpty())
        {
            search.clear();
            model.filter.query.clear();
            filterChanged();
        }
        giveFocusBack();
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
        if (proc.isLoading())   // rendering now would decode the samples on the UI thread
        {
            showStatus (juce::String (juce::CharPointer_UTF8 ("Samples are still loading\xe2\x80\xa6 drag again in a moment")));
            return;
        }
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
    const int right = randomBtn.getX() - 10 - curW - 12;
    return { left, 14, right - left, 38 };
}
juce::Rectangle<int> VaultMainView::curRect() const { return { randomBtn.getX() - 10 - 268, 15, 268, 36 }; }
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
        g.setGradientFill (chromeGradient (9.0f, 34.0f));
        g.setFont (font (22.0f, true, 0.14f));
        g.drawText ("CHROME VAULT", 20, 8, logoWidth() + 10, 28, Justification::centredLeft, false);
        g.setColour (col::dim);
        g.setFont (font (14.0f, true, 0.2f));
        g.drawText ("BY TRAPWIN", 21, 36, 200, 20, Justification::centredLeft, false);
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
        g.setFont (font (14.5f, true));
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
            const auto f = font (14.0f, strong);
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
            put (midDot() + statusHint(), false);
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
        proc.previewNote (vault::homeNote, 0.8f, p->category == vault::catPads || p->category == vault::catFxAtmos ? 1.4 : 0.9);   // C5 for every sound
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
    const int st = proc.isMissingSamples() ? 2 : proc.isLoading() ? 1 : 0;
    if (st != lastLoadState) { lastLoadState = st; repaint (statusRect()); info.repaint(); }
    else if (st == 1) info.repaintStatus();   // the loading spinners turn (the UI never waits for the samples)
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
    // nobody in the plugin keeps the keyboard: the host gets the keys again (its computer-keyboard piano, shortcuts)
    ++focusReturnCount;
    search.giveAwayKeyboardFocus();
    unfocusAllComponents();
    repaint (searchRect().expanded (2));
}

juce::String VaultMainView::statusHint() const
{
    if (proc.currentPresetData() != nullptr)
    {
        if (proc.isMissingSamples())
            return proc.libraryFound() ? juce::String (juce::CharPointer_UTF8 ("This sound needs a newer sound library \xe2\x80\x94 its synth layers play"))
                                       : juce::String (juce::CharPointer_UTF8 ("Sound library not found \xe2\x80\x94 synth sounds play, sampled layers are silent"));
        if (proc.isLoading()) return juce::String (juce::CharPointer_UTF8 ("Loading samples\xe2\x80\xa6 the sound plays as soon as they are in"));
    }
    return "Click a sound to hear it at C5" + midDot() + juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x80 \xe2\x96\xb6 next sound")) + midDot() + "Instant load";
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
    // sizes that would not fit on this screen are greyed out (the window must never be larger than the screen)
    const auto* ed = findParentComponentOfClass<VaultEditor>();
    const auto area = ed != nullptr ? ed->displayArea() : juce::Rectangle<int>();
    int id = 1;
    for (float s : VaultEditor::scalePresets())
    {
        const bool ticked = std::abs (s - cur) < 0.005f;
        const bool ok = area.isEmpty() || ticked || s <= VaultEditor::minScale || VaultEditor::fits (s, area);
        m.addItem (id++, juce::String (juce::roundToInt (s * 100.0f)) + " %", ok, ticked);
    }
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
    if (p == nullptr || proc.isLoading()) return {};   // never decode samples on the UI thread
    float macros[vault::numMacros];
    for (int i = 0; i < vault::numMacros; ++i) macros[i] = proc.getMacro (i);
    constexpr double sr = 44100.0;
    const auto buf = VaultEngine::render (*p, vault::homeNote, 0.8f, 2.0, sr, macros);   // the preview note: C5 (MIDI 60) for every sound
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("CHROME VAULT");
    dir.createDirectory();
    const auto f = dir.getChildFile (juce::File::createLegalFileName (p->name + " " + vault::noteName (vault::homeNote)) + ".wav");
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
    static const std::vector<float> s { 0.8f, 0.9f, 1.0f, 1.15f, 1.3f, 1.5f, 1.75f, 2.0f };
    return s;
}

bool VaultEditor::fits (float scale, juce::Rectangle<int> area)
{
    return juce::roundToInt ((float) baseWidth * scale) <= area.getWidth() && juce::roundToInt ((float) baseHeight * scale) <= area.getHeight();
}

float VaultEditor::firstOpenScale (juce::Rectangle<int> usable)
{
    if (usable.isEmpty()) return 1.0f;
    const auto area = (usable.toFloat() * 0.85f).toNearestInt();   // 85 % of the screen: room for the DAW's own window frame
    float s = minScale;
    for (float c : scalePresets())
        if (c <= 1.0f && fits (c, area)) s = c;
    return s;
}

float VaultEditor::restoredScale (float saved, juce::Rectangle<int> usable)
{
    saved = juce::jlimit (minScale, maxScale, saved);
    if (usable.isEmpty() || fits (saved, usable)) return saved;
    float s = minScale;   // a screen smaller than 80 % gets 80 % (the smallest readable size)
    for (float c : scalePresets())
        if (c < saved && fits (c, usable)) s = c;
    return s;
}

juce::Rectangle<int> VaultEditor::displayArea() const
{
    if (! simulatedDisplay().isEmpty()) return simulatedDisplay();
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const juce::Displays::Display* d = nullptr;
    if (isShowing()) d = displays.getDisplayForRect (getScreenBounds());
    if (d == nullptr) d = displays.getPrimaryDisplay();
    return d != nullptr ? d->userBounds.getSmallestIntegerContainer() : juce::Rectangle<int>();
}

VaultEditor::VaultEditor (VaultProcessor& p) : AudioProcessorEditor (p), proc (p), settings (VaultSettings::get())
{
    setLookAndFeel (&lnf);
    main = std::make_unique<VaultMainView> (proc, settings);
    main->currentScale = [this] { return uiScale; };
    main->onScaleChosen = [this] (float s) { setUiScale (s); };
    addAndMakeVisible (*main);

    // first open: the largest preset up to 100 % that fits 85 % of the screen; later the remembered scale, but never a
    // window larger than the screen (e.g. 200 % chosen on a 4K screen, now opened on a laptop) - the choice itself stays saved
    const float saved = settings->scale();
    const auto area = displayArea();
    const float s = saved <= 0.0f ? firstOpenScale (area) : restoredScale (saved, area);

    // never take the keyboard: the DAW's computer-keyboard piano must keep working (only the search box takes it while typing)
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
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
    main.reset();
    settings->flush();
    setLookAndFeel (nullptr);
}

void VaultEditor::setUiScale (float s)
{
    s = juce::jlimit (minScale, maxScale, s);
    setSize (juce::roundToInt (baseWidth * s), juce::roundToInt (baseHeight * s));
    settings->setScale (s);   // the user's choice (also when it equals the first-open size)
}

void VaultEditor::applyScale (float s)
{
    s = juce::jlimit (minScale, maxScale, s);
    const juce::ScopedValueSetter<bool> svs (settingSize, true);
    setSize (juce::roundToInt (baseWidth * s), juce::roundToInt (baseHeight * s));
}

void VaultEditor::fitToDisplay()
{
    // the constructor only knew the main display; the editor may have opened on another one
    const auto area = displayArea();
    if (area.isEmpty()) return;
    const float saved = settings->scale();
    const float target = saved <= 0.0f ? firstOpenScale (area) : restoredScale (saved, area);
    if (std::abs (target - uiScale) > 0.001f) applyScale (target);
}

void VaultEditor::paint (juce::Graphics& g) { g.fillAll (vui::col::ink); }

void VaultEditor::resized()
{
    if (main == nullptr) return;
    // the scale that fits both sides (a host may ignore the aspect ratio), the view centred in any leftover space
    const float fit = std::min ((float) getWidth() / (float) baseWidth, (float) getHeight() / (float) baseHeight);
    const float old = uiScale;
    uiScale = juce::jlimit (minScale, maxScale, fit);
    const float dx = std::max (0.0f, ((float) getWidth() - (float) baseWidth * uiScale) * 0.5f);
    const float dy = std::max (0.0f, ((float) getHeight() - (float) baseHeight * uiScale) * 0.5f);
    main->setBounds (0, 0, baseWidth, baseHeight);
    main->setTransform (juce::AffineTransform::scale (uiScale).translated (std::round (dx), std::round (dy)));
    if (! settingSize && std::abs (uiScale - old) > 0.001f) settings->setScale (uiScale);   // corner drag / host resize
    main->updateToggles();
    main->repaint();
}

void VaultEditor::timerCallback()
{
    if (main == nullptr) return;
    main->syncFromProcessor();
    if (! displayChecked && isShowing())
    {
        displayChecked = true;
        fitToDisplay();
    }
}

void VaultEditor::mouseDown (const juce::MouseEvent& e)
{
    // a click anywhere but in the search box ends typing: the keyboard goes back to the DAW
    if (main != nullptr && main->searchHasFocus() && e.eventComponent != nullptr
        && ! main->searchBox().isParentOf (e.eventComponent) && e.eventComponent != &main->searchBox())
        main->giveFocusBack();
}
