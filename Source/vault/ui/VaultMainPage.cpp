#include "VaultPages.h"
#include "../VaultEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

// CHROME VAULT v3 MAIN page (Nexus-style): FILTER · AMP + MOTION | the browser screen + 4 sound macros | EFFECTS + OUTPUT
namespace vui
{
namespace
{
// page-local layout (1148 x 612)
constexpr int sideW = 200, gapX = 14, screenX = sideW + gapX, screenW = 1148 - 2 * (sideW + gapX), macroH = 100, gapY = 10;
constexpr int cellW = 96, cellPitch = 96, gridTop = 38, knobD = 44;
constexpr int screenH = 612 - macroH - gapY;
constexpr int pad = 10, rowTop = pad, rowH = 30, colsTop = rowTop + rowH + 8, curH = 52;
constexpr int catsW = 198;

float quickAmount (float designed, float q, float top)
{
    if (std::abs (q - 0.5f) < 1.0e-6f) return designed;
    if (q < 0.5f) return designed * (2.0f * q);
    return designed + (std::max (designed, top) - designed) * (2.0f * q - 1.0f);
}
bool isFxQuick (int q) { return q >= vault::qReverb && q <= vault::qEqTilt; }
vault::FxType fxOfQuick (int q)
{
    switch (q)
    {
        case vault::qReverb: return vault::FxType::reverb;
        case vault::qDelay:  return vault::FxType::delay;
        case vault::qChorus: return vault::FxType::chorus;
        case vault::qDrive:  return vault::FxType::drive;
        case vault::qWidth:  return vault::FxType::width;
        default:             return vault::FxType::eq;
    }
}
const char* quickLabel (int q)
{
    static const char* n[vault::numQuick] { "CUTOFF", "RESO", "ATTACK", "RELEASE", "ENV AMT", "GLIDE", "VIBRATO", "TREMOLO",
                                            "REVERB", "DELAY", "CHORUS", "DRIVE", "WIDTH", "EQ TILT", "PAN" };
    return n[juce::jlimit (0, (int) vault::numQuick - 1, q)];
}
juce::String times() { return juce::String (juce::CharPointer_UTF8 ("\xc3\x97")); }

// remove any focus-on-click from a component tree (focus stays with the editor unless the search box is clicked)
void noClickFocus (juce::Component& c, const juce::Component* except)
{
    if (&c == except) return;
    c.setMouseClickGrabsKeyboardFocus (false);
    c.setWantsKeyboardFocus (false);
    for (auto* ch : c.getChildren()) noClickFocus (*ch, except);
}
} // namespace

juce::String MainPage::quickTarget (int q)
{
    switch (q)
    {
        case vault::qCutoff: return "cutoff";
        case vault::qReso:   return "reso";
        case vault::qPan:    return "pan";
        case vault::qReverb: return "fx.reverb.mix";
        case vault::qDelay:  return "fx.delay.mix";
        case vault::qChorus: return "fx.chorus.mix";
        case vault::qDrive:  return "fx.drive.amount";
        case vault::qWidth:  return "fx.width.width";
        case vault::qEqTilt: return "fx.eq.tilt";
        default:             return juce::String ("quick.") + vault::quickInfo()[(size_t) q].id;
    }
}

MainPage::MainPage (UiContext& c)
    : Page (c), model (c.proc.bank(), c.proc.sampleLibrary()), cats (model), list (c.proc.bank(), model)
{
    auto& proc = ctx.proc;
    model.setFavoriteIds (ctx.settings->favorites());
    model.setRecentIds (ctx.settings->recent());
    model.refilter();

    // ---- screen
    search.setFont (font (14.0f));
    search.setTextToShowWhenEmpty ("Search " + thousands (model.size()) + " sounds", col::dim);
    search.setIndents (0, 0);
    search.setJustification (juce::Justification::centredLeft);
    search.setSelectAllWhenFocused (false);
    search.setEscapeAndReturnKeysConsumed (true);
    search.setTitle ("Search sounds");
    search.onTextChange = [this] { model.filter.query = search.getText(); filterChanged(); };
    search.onEscapeKey = [this]
    {
        if (! search.isEmpty()) { search.clear(); model.filter.query.clear(); filterChanged(); }
        giveFocusBack();
    };
    search.onReturnKey = [this] { if (model.rowOf (shown) < 0 && visibleCount() > 0) loadPreset (model.visible().front(), true); giveFocusBack(); };
    search.addKeyListener (&searchKeys);
    search.onFocusLost = [this] { repaint (searchRect().expanded (2)); };
    addAndMakeVisible (search);

    prevBtn.onClick = [this] { step (-1, true); };
    nextBtn.onClick = [this] { step (1, true); };
    favBtn.onClick = [this] { setFavoritesOnly (! model.filter.favoritesOnly); };
    recentBtn.onClick = [this] { setRecentOnly (! model.filter.recentOnly); };
    moreBtn.onClick = [this] { moreLikeThis(); };
    heartBtn.onClick = [this] { if (shown >= 0) toggleFavorite (shown); };
    heartBtn.setTooltip ("Favorite");
    for (auto* b : std::initializer_list<juce::Component*> { &prevBtn, &nextBtn, &favBtn, &recentBtn, &moreBtn, &wavBtn, &heartBtn }) addAndMakeVisible (b);
    cats.onSelect = [this] (int cat) { setCategory (cat); };
    addAndMakeVisible (cats);
    list.onClick = [this] (int idx) { loadPreset (idx, true); };
    list.onToggleFavorite = [this] (int idx) { toggleFavorite (idx); };
    addAndMakeVisible (list);

    wavBtn.onDragOut = [this]
    {
        if (ctx.proc.isLoading())   // rendering now would decode the samples on the UI thread
        {
            showStatus (juce::String (juce::CharPointer_UTF8 ("Samples are still loading\xe2\x80\xa6 drag again in a moment")));
            return;
        }
        const auto f = renderWavForDrag();
        if (f.existsAsFile())
        {
            showStatus ("Drop the WAV into your DAW or a folder: " + f.getFileName());
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, &wavBtn);
        }
        else showStatus ("Could not write the WAV file.");
    };
    wavBtn.onClickOnly = [this] { showStatus ("Drag this button into your DAW or a folder: the sound at C5 as a 2 s WAV."); };

    // ---- knobs
    for (int q = 0; q < vault::numQuick; ++q)
    {
        auto& k = quick[(size_t) q];
        k.label = quickLabel (q);
        k.diameter = knobD;
        k.resetValue = vault::quickInfo()[(size_t) q].neutral;
        k.arcOrigin = vault::quickInfo()[(size_t) q].neutral;   // offsets (0.5 = as designed) are bipolar
        k.valueText = [this, q] (float v) { return quickText (q, v); };
        k.setTooltip (vault::quickInfo()[(size_t) q].name + juce::String (": ") + vault::quickInfo()[(size_t) q].what);
        if (isFxQuick (q))
        {
            k.hasLed = true;
            k.onLedClick = [this, q]
            {
                ctx.proc.setQuickFxOn (q, ! ctx.proc.isQuickFxOn (q));
                if (ctx.soundEdited) ctx.soundEdited();
            };
        }
        k.onRightClick = [this, q] (const juce::MouseEvent&) { mod::showMenu (ctx, quick[(size_t) q], quickTarget (q)); };
        addAndMakeVisible (k);
        if (auto* p = proc.apvts.getParameter (VaultProcessor::idQuick[q])) atts.push_back (std::make_unique<KnobAttachment> (*p, k));
    }
    for (int i = 0; i < vault::numMacros; ++i)
    {
        auto& k = macro[(size_t) i];
        k.diameter = 42;
        k.valueText = [] (float v) { return pct (v); };
        addAndMakeVisible (k);
        if (auto* p = proc.apvts.getParameter (VaultProcessor::idMacro[i])) atts.push_back (std::make_unique<KnobAttachment> (*p, k));
    }
    volume.diameter = knobD;
    volume.resetValue = proc.apvts.getParameter (VaultProcessor::idVolume)->convertTo0to1 (0.0f);
    volume.valueText = [this] (float v)
    {
        const float db = ctx.proc.apvts.getParameter (VaultProcessor::idVolume)->convertFrom0to1 (v);
        return db <= -59.9f ? juce::String ("-inf dB") : juce::String (db, 1) + " dB";
    };
    volume.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, volume, "volume"); };
    addAndMakeVisible (volume);
    if (auto* p = proc.apvts.getParameter (VaultProcessor::idVolume)) atts.push_back (std::make_unique<KnobAttachment> (*p, volume));

    ctx.settings->addChangeListener (this);
    noClickFocus (*this, &search);
    updateToggles();
    const int cur = proc.currentPreset();
    setShown (cur);
    list.showRow (model.rowOf (cur));
    refresh();
}

MainPage::~MainPage()
{
    search.removeKeyListener (&searchKeys);
    ctx.settings->removeChangeListener (this);
    atts.clear();
}

//==============================================================================
juce::Rectangle<int> MainPage::leftRect() const   { return { 0, 0, sideW, getHeight() }; }
juce::Rectangle<int> MainPage::rightRect() const  { return { getWidth() - sideW, 0, sideW, getHeight() }; }
juce::Rectangle<int> MainPage::screenRect() const { return { screenX, 0, screenW, screenH }; }
juce::Rectangle<int> MainPage::macroPanelRect() const { return { screenX, screenH + gapY, screenW, macroH }; }
juce::Rectangle<int> MainPage::searchRect() const
{
    return { prevBtn.getRight() + 6, screenRect().getY() + rowTop, nextBtn.getX() - 6 - (prevBtn.getRight() + 6), rowH };
}
juce::Rectangle<int> MainPage::catsRect() const { return { screenX + pad, colsTop, catsW, screenH - pad - curH - 8 - colsTop }; }
juce::Rectangle<int> MainPage::listRect() const
{
    const auto c = catsRect();
    return { c.getRight() + 8, c.getY(), screenX + screenW - pad - (c.getRight() + 8), c.getHeight() };
}
juce::Rectangle<int> MainPage::curRect() const { return { screenX + pad, screenH - pad - curH, screenW - 2 * pad, curH }; }
juce::Rectangle<int> MainPage::noticeRect() const { return listRect().removeFromBottom (104).reduced (10, 10); }
juce::Rectangle<float> MainPage::heartRect() const { return heartBtn.getBounds().toFloat(); }

void MainPage::resized()
{
    // screen: ◀ [search] ▶ Favorites Recent
    {
        const auto s = screenRect();
        const int y = s.getY() + rowTop;
        int x = s.getRight() - pad;
        auto place = [&] (juce::Component& b, int w) { x -= w; b.setBounds (x, y, w, rowH); x -= 6; };
        place (recentBtn, recentBtn.idealWidth());
        place (favBtn, favBtn.idealWidth());
        place (nextBtn, rowH);
        prevBtn.setBounds (s.getX() + pad, y, rowH, rowH);
        search.setBounds (searchRect().withTrimmedLeft (32).withTrimmedRight (10).reduced (0, 4));
    }
    cats.setBounds (catsRect().reduced (0, 0));
    cats.rowH = (catsRect().getHeight() - 2 * CategoryList::padTop) / (vault::numCategories + 1);
    list.setBounds (listRect());
    {
        auto c = curRect();
        int x = c.getRight();
        const int y = c.getY() + 14;
        auto place = [&] (juce::Component& b, int w) { x -= w; b.setBounds (x, y, w, 30); x -= 8; };
        place (heartBtn, 34);
        place (wavBtn, wavBtn.idealWidth());
        place (moreBtn, moreBtn.idealWidth());
    }
    // side columns: 2 knobs per row
    auto grid = [&] (juce::Rectangle<int> col, std::initializer_list<int> qs, int top)
    {
        int i = 0;
        for (int q : qs)
        {
            auto& k = q >= 0 ? quick[(size_t) q] : volume;
            k.setBounds (col.getX() + 4 + (i % 2) * cellW, top + (i / 2) * cellPitch, cellW, k.idealHeight());
            ++i;
        }
    };
    const auto l = leftRect(), r = rightRect();
    grid (l, { vault::qCutoff, vault::qReso, vault::qAttack, vault::qRelease, vault::qEnvAmt, vault::qGlide }, gridTop);
    grid (l, { vault::qVibrato, vault::qTremolo }, gridTop + 3 * cellPitch + 26);
    grid (r, { vault::qReverb, vault::qDelay, vault::qChorus, vault::qDrive, vault::qWidth, vault::qEqTilt }, gridTop);
    grid (r, { -1, vault::qPan }, gridTop + 3 * cellPitch + 26);
    // sound macros
    const auto m = macroPanelRect();
    for (int i = 0; i < vault::numMacros; ++i)
        macro[(size_t) i].setBounds (m.getX() + 14 + i * 112, m.getY() + 10, 112, macro[(size_t) i].idealHeight());
}

void MainPage::paint (juce::Graphics& g)
{
    using namespace juce;
    // side panels + headers
    for (auto r : { leftRect(), rightRect() }) drawPanel (g, r.toFloat());
    const float h2 = (float) (gridTop + 3 * cellPitch + 2);
    drawHeader (g, "FILTER" + upDot() + "AMP", leftRect().toFloat().withHeight (gridTop).withTrimmedTop (8), Justification::centred);
    drawHeader (g, "MOTION", Rectangle<float> ((float) leftRect().getX(), h2, (float) sideW, 22.0f), Justification::centred);
    drawHeader (g, "EFFECTS", rightRect().toFloat().withHeight (gridTop).withTrimmedTop (8), Justification::centred);
    drawHeader (g, "OUTPUT", Rectangle<float> ((float) rightRect().getX(), h2, (float) sideW, 22.0f), Justification::centred);

    // the screen
    {
        const auto s = screenRect().toFloat();
        g.setColour (Colour (0xff121925));
        g.fillRoundedRectangle (s.expanded (3.0f), 14.0f);
        g.setColour (col::screen);
        g.fillRoundedRectangle (s, 12.0f);
        g.setGradientFill (ColourGradient (col::deep.withAlpha (0.08f), 0, s.getY(), Colours::transparentBlack, 0, s.getY() + 40.0f, false));
        g.fillRoundedRectangle (s, 12.0f);
        g.setColour (col::screenRim);
        g.drawRoundedRectangle (s.reduced (0.5f), 12.0f, 1.0f);
        // search field
        const auto sr = searchRect().toFloat();
        g.setColour (Colour (0xff070b11));
        g.fillRoundedRectangle (sr, 6.0f);
        g.setColour (search.hasKeyboardFocus (true) ? col::ice2 : col::pillLine);
        g.drawRoundedRectangle (sr.reduced (0.5f), 6.0f, 1.0f);
        drawSearchIcon (g, { sr.getX() + 11.0f, sr.getCentreY() - 7.0f, 14.0f, 14.0f }, col::dim);
        // wells
        for (auto w : { catsRect().toFloat(), listRect().toFloat() })
        {
            g.setColour (col::well);
            g.fillRoundedRectangle (w, 8.0f);
            g.setColour (col::wellLine);
            g.drawRoundedRectangle (w.reduced (0.5f), 8.0f, 1.0f);
        }
        // current sound
        const auto c = curRect().toFloat();
        g.setColour (Colour (0xff121a26));
        g.fillRect (c.getX(), c.getY() - 4.0f, c.getWidth(), 1.0f);
        const auto* p = ctx.proc.currentPresetData();
        const float textR = (float) moreBtn.getX() - 12.0f;
        auto nameR = Rectangle<float> (c.getX() + 4.0f, c.getY() + 2.0f, textR - c.getX() - 4.0f, 26.0f);
        if (p != nullptr)
        {
            const auto nf = font (19.0f, true);
            g.setFont (nf);
            g.setColour (col::text);
            g.drawText (p->name, nameR, Justification::centredLeft, true);
            float x = nameR.getX() + std::min (nameR.getWidth(), textWidth (nf, p->name)) + 10.0f;
            const auto st = notice();
            if (st == Notice::loading)
            {
                drawSpinner (g, { x, nameR.getCentreY() - 7.0f, 14.0f, 14.0f }, col::ice2);
                g.setFont (font (14.0f));
                g.setColour (col::muted);
                g.drawText (String (CharPointer_UTF8 ("Loading\xe2\x80\xa6")), Rectangle<float> (x + 20.0f, nameR.getY(), 90.0f, nameR.getHeight()), Justification::centredLeft, false);
            }
            else if (ctx.proc.isSoundEdited())
            {
                g.setFont (font (14.0f));
                g.setColour (col::ice2);
                g.drawText ("edited", Rectangle<float> (x, nameR.getY() + 1.0f, 70.0f, nameR.getHeight()), Justification::centredLeft, false);
            }
            const bool msg = statusMsg.isNotEmpty() && Time::getMillisecondCounter() < statusUntil;
            g.setFont (font (14.0f));
            g.setColour (msg ? col::ice : col::dim);
            g.drawText (msg ? statusMsg : composition(), Rectangle<float> (c.getX() + 4.0f, c.getY() + 28.0f, textR - c.getX() - 4.0f, 20.0f),
                        Justification::centredLeft, true);
        }
        else
        {
            g.setFont (font (15.0f));
            g.setColour (col::dim);
            g.drawText ("No sound loaded", nameR, Justification::centredLeft, false);
        }
    }

    // sound macros
    {
        const auto m = macroPanelRect();
        drawPanel (g, m.toFloat());
        const int tx = m.getX() + 14 + 4 * 112 + 14;
        auto t = Rectangle<float> ((float) tx, (float) m.getY() + 18.0f, (float) (m.getRight() - 16 - tx), 20.0f);
        drawHeader (g, "SOUND MACROS", t);
        g.setFont (font (14.0f));
        g.setColour (col::dim);
        const auto lines = wrapText (font (14.0f), "set by the sound designer for each preset", t.getWidth());
        float y = t.getBottom() + 4.0f;
        for (auto& ln : lines) { g.drawText (ln, Rectangle<float> (t.getX(), y, t.getWidth() + 2.0f, 19.0f), Justification::centredLeft, false); y += 19.0f; }
    }
}

void MainPage::paintOverChildren (juce::Graphics& g)
{
    using namespace juce;
    const auto st = notice();
    if (st != Notice::libraryMissing && st != Notice::instrumentMissing) return;
    // a calm card over the bottom of the list: what is missing, what to do, where it was looked for
    const auto r = noticeRect().toFloat();
    g.setColour (Colour (0xf20a1019));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (col::fieldLine);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (col::ice2.withAlpha (0.7f));
    g.fillRoundedRectangle (r.getX() + 7.0f, r.getY() + 10.0f, 2.5f, r.getHeight() - 20.0f, 1.25f);
    auto t = r.reduced (16.0f, 6.0f).withTrimmedLeft (4.0f);
    const float lineH = 19.0f;
    bool anySynth = false;
    if (const auto* p = ctx.proc.currentPresetData()) for (auto& part : p->parts) anySynth |= part.kind != VaultPartKind::sample;
    StringArray lines; std::vector<bool> strong;
    for (auto& l : wrapText (font (14.0f, true), noticeTitle(), t.getWidth())) { lines.add (l); strong.push_back (true); }
    for (auto& l : wrapText (font (14.0f), anySynth ? "Its synth layers still play." : "Synth sounds keep playing.", t.getWidth())) { lines.add (l); strong.push_back (false); }
    for (auto& l : wrapText (font (14.0f), ctx.proc.libraryPath().getFullPathName(), t.getWidth())) { lines.add (l); strong.push_back (false); }
    float y = t.getY() + std::max (0.0f, (t.getHeight() - lineH * (float) lines.size()) * 0.5f);
    for (int i = 0; i < lines.size(); ++i)
    {
        if (y + lineH > t.getBottom() + 0.5f) break;
        g.setFont (font (14.0f, strong[(size_t) i]));
        g.setColour (strong[(size_t) i] ? col::text : i == lines.size() - 1 ? col::dim : col::muted);
        g.drawText (lines[i], Rectangle<float> (t.getX(), y, t.getWidth() + 2.0f, lineH), Justification::centredLeft, false);
        y += lineH;
    }
}

//==============================================================================
juce::String MainPage::quickText (int q, float v) const
{
    const auto& snd = ctx.sound();
    const bool neutral = std::abs (v - vault::quickInfo()[(size_t) q].neutral) < 0.004f;
    switch (q)
    {
        case vault::qCutoff: return neutral ? juce::String ("0 oct") : (v > 0.5f ? "+" : "") + juce::String ((v - 0.5f) * 8.0f, 1) + " oct";
        case vault::qReso: case vault::qEnvAmt: case vault::qGlide:
            return neutral ? juce::String ("0%") : signedPct ((v - 0.5f) * 2.0f);
        case vault::qAttack: case vault::qRelease:
        {
            if (neutral) return times() + "1.0";
            const float mul = std::exp2 ((v - 0.5f) * 8.0f);
            return times() + (mul >= 9.95f ? juce::String (juce::roundToInt (mul)) : juce::String (mul, mul < 0.1f ? 2 : 1));
        }
        case vault::qVibrato: return v < 0.004f ? juce::String ("off") : juce::String (juce::roundToInt (v * 60.0f)) + " ct";
        case vault::qTremolo: return v < 0.004f ? juce::String ("off") : pct (v);
        case vault::qPan:
        {
            const int p = juce::roundToInt ((v - 0.5f) * 200.0f);
            return p == 0 ? juce::String ("C") : (p < 0 ? "L " : "R ") + juce::String (std::abs (p));
        }
        default: break;
    }
    // EFFECTS: the effective amount of the chain's slot (or the default slot the knob uses) + its type
    const auto t = fxOfQuick (q);
    const int slot = ctx.proc.fxSlotForQuick (q);
    const VaultFxSlot* s = slot >= 0 ? &snd.fx[(size_t) slot] : nullptr;
    const bool on = ctx.proc.isQuickFxOn (q) && ctx.proc.getSwitch (VaultProcessor::idFxOn);
    if (! on) return "off";
    const auto& ti = vault::fxTypeInfo (t);
    const float designed = s != nullptr ? s->p[0] : (t == vault::FxType::width ? 0.5f : 0.0f);
    switch (q)
    {
        case vault::qReverb: return pct (quickAmount (designed, v, 0.7f)) + " " + choiceName (ti.params[2], s != nullptr ? s->p[2] : 0.0f);
        case vault::qDelay:
        {
            const bool sync = s == nullptr || s->get ("sync") > 0.5f;
            const auto tm = sync ? choiceName (ti.params[1], s != nullptr ? s->p[1] : ti.params[1].def)
                                 : juce::String (juce::roundToInt (s->get ("ms"))) + "ms";
            return pct (quickAmount (designed, v, 0.6f)) + " " + tm;
        }
        case vault::qChorus: return pct (quickAmount (designed, v, 1.0f));
        case vault::qDrive:  return pct (quickAmount (designed, v, 0.8f)) + " " + choiceName (ti.params[1], s != nullptr ? s->p[1] : 0.0f);
        case vault::qWidth:  return pct (quickAmount (designed, v, 1.0f));
        case vault::qEqTilt:
        {
            const float db = juce::jlimit (-12.0f, 12.0f, designed + (v - 0.5f) * 18.0f);
            return (db > 0.05f ? "+" : "") + juce::String (db, 1) + " dB";
        }
        default: return {};
    }
}

void MainPage::refresh()
{
    const auto& snd = ctx.sound();
    const bool fxOn = ctx.proc.getSwitch (VaultProcessor::idFxOn);
    for (int q = 0; q < vault::numQuick; ++q)
    {
        auto& k = quick[(size_t) q];
        if (k.hasLed) { const bool on = fxOn && ctx.proc.isQuickFxOn (q); if (on != k.ledOn) { k.ledOn = on; k.repaint(); } }
        mod::show (k, snd, quickTarget (q));
        k.repaint();   // value texts follow the sound (types, designed amounts)
    }
    mod::show (volume, snd, "volume");
    const auto& bank = ctx.bankSound();
    for (int i = 0; i < vault::numMacros; ++i)
    {
        auto& k = macro[(size_t) i];
        const auto name = ctx.proc.macroName (i).trim();
        const auto label = name.isNotEmpty() ? name.toUpperCase() : juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));
        if (k.label != label) { k.label = label; k.repaint(); }
        k.setEnabledLook (name.isNotEmpty());
        k.setTooltip (name.isNotEmpty() ? "Sound macro: " + name : juce::String ("Not used by this sound"));
        k.resetValue = bank.macroDefaults[i];
    }
    repaint (curRect().expanded (2));
}

void MainPage::tick()
{
    if (statusUntil != 0 && juce::Time::getMillisecondCounter() >= statusUntil) { statusUntil = 0; repaint (curRect()); }
}

//==============================================================================
MainPage::Notice MainPage::notice() const
{
    if (shown < 0 || shown != ctx.proc.currentPreset()) return Notice::none;
    if (ctx.proc.isMissingSamples()) return ctx.proc.libraryFound() ? Notice::instrumentMissing : Notice::libraryMissing;
    if (ctx.proc.isLoading()) return Notice::loading;
    return Notice::none;
}

MainPage::PartState MainPage::partState (const VaultPart& part) const
{
    if (part.kind != VaultPartKind::sample) return PartState::ready;
    const auto* lib = ctx.proc.sampleLibrary();
    const auto* h = lib != nullptr && lib->libraryFound() ? lib->find (part.source) : nullptr;
    if (h == nullptr || h->hasFailed()) return PartState::missing;
    return ! h->isLoaded() && shown == ctx.proc.currentPreset() ? PartState::loading : PartState::ready;
}

juce::String MainPage::noticeTitle() const
{
    switch (notice())
    {
        case Notice::libraryMissing:    return juce::String (juce::CharPointer_UTF8 ("Sound library not found \xe2\x80\x94 run the CHROME VAULT installer"));
        case Notice::instrumentMissing: return juce::String (juce::CharPointer_UTF8 ("This sound needs a newer sound library \xe2\x80\x94 run the latest CHROME VAULT installer"));
        case Notice::none: case Notice::loading: default: return {};
    }
}

juce::String MainPage::composition() const
{
    const auto* p = ctx.proc.currentPresetData();
    if (p == nullptr) return {};
    const float maxW = (float) (moreBtn.getX() - 12 - curRect().getX() - 4);
    juce::String out = vault::categoryNames()[juce::jlimit (0, (int) vault::numCategories - 1, p->category)];
    const auto f = font (14.0f);
    juce::StringArray names;
    for (auto& part : p->parts)
    {
        auto n = VaultBrowserModel::instrumentName (part, ctx.proc.sampleLibrary());
        const auto label = part.label.trim();
        if (label.isNotEmpty() && part.kind != VaultPartKind::sample) n = label;
        if (partState (part) == PartState::missing) n << " (not installed)";
        names.add (n);
    }
    for (int i = 0; i < names.size(); ++i)
    {
        const auto next = out + (i == 0 ? midDot() : juce::String (" + ")) + names[i];
        const auto more = " + " + juce::String (names.size() - i - 1) + " more";
        if (textWidth (f, next + (i < names.size() - 1 ? more : juce::String())) > maxW && i > 0)
        {
            out << " + " << juce::String (names.size() - i) << " more";
            break;
        }
        out = next;
    }
    return out;
}

juce::String MainPage::statusHint() const
{
    if (ctx.proc.currentPresetData() != nullptr)
    {
        if (ctx.proc.isMissingSamples())
            return ctx.proc.libraryFound() ? juce::String (juce::CharPointer_UTF8 ("This sound needs a newer sound library \xe2\x80\x94 its synth layers play"))
                                           : juce::String (juce::CharPointer_UTF8 ("Sound library not found \xe2\x80\x94 synth sounds play, sampled layers are silent"));
        if (ctx.proc.isLoading()) return juce::String (juce::CharPointer_UTF8 ("Loading samples\xe2\x80\xa6 the sound plays as soon as they are in"));
    }
    return "Click a sound to hear it at C5";
}

//==============================================================================
void MainPage::setShown (int idx)
{
    shown = idx;
    list.setCurrent (idx);
    heartBtn.setToggleState (idx >= 0 && model.isFavorite (idx), juce::dontSendNotification);
    repaint (curRect().expanded (4));
}

void MainPage::loadPreset (int idx, bool audition)
{
    if (! juce::isPositiveAndBelow (idx, model.size())) return;
    ctx.proc.selectPreset (idx);
    setShown (idx);
    model.pushRecent (idx);
    ctx.settings->setRecent (model.recentIds());
    list.showRow (model.rowOf (idx));
    if (ctx.soundEdited) ctx.soundEdited();
    if (audition && ctx.settings->previewOnClick()) auditionCurrent();
}

void MainPage::auditionCurrent()
{
    if (const auto* p = ctx.proc.currentPresetData())
        ctx.proc.previewNote (vault::homeNote, 0.8f, p->category == vault::catPads || p->category == vault::catFxAtmos ? 1.4 : 0.9);   // C5 for every sound
}

void MainPage::step (int delta, bool wrap)
{
    const int next = model.step (shown, delta, wrap);
    if (next >= 0 && next != shown) loadPreset (next, true);
}

void MainPage::syncFromProcessor()
{
    const int cur = ctx.proc.currentPreset();
    if (cur != shown)
    {
        setShown (cur);
        list.showRow (model.rowOf (cur));
        if (ctx.soundEdited) ctx.soundEdited();
    }
    const int st = ctx.proc.isMissingSamples() ? 2 : ctx.proc.isLoading() ? 1 : 0;
    if (st != lastLoadState) { lastLoadState = st; repaint(); }
    else if (st == 1) repaint (curRect());   // the loading spinner turns (the UI never waits for the samples)
}

void MainPage::filterChanged (bool scrollToCurrent, bool keepScroll)
{
    model.refilter();
    list.refresh();
    const int row = model.rowOf (shown);
    if (scrollToCurrent && row >= 0) list.showRow (row);
    else if (! keepScroll) list.scrollToTop();
    cats.setSelected (model.filter.category);
    updateToggles();
}

void MainPage::setCategory (int c) { model.filter.category = juce::jlimit (-1, (int) vault::numCategories - 1, c); filterChanged(); }
void MainPage::setQuery (const juce::String& q) { search.setText (q, false); model.filter.query = q; filterChanged(); }
void MainPage::setLayers (int n) { model.filter.layers = juce::jlimit (0, (int) vault::maxParts, n); filterChanged(); }
void MainPage::setFavoritesOnly (bool on) { model.filter.favoritesOnly = on; if (on) model.filter.recentOnly = false; filterChanged(); }
void MainPage::setRecentOnly (bool on) { model.filter.recentOnly = on; if (on) model.filter.favoritesOnly = false; filterChanged (! on); }

void MainPage::moreLikeThis()
{
    if (shown < 0) return;
    const auto& p = ctx.proc.bank()[(size_t) shown];
    model.filter.favoritesOnly = model.filter.recentOnly = false;
    model.filter.layers = 0;
    model.filter.category = p.category;
    const auto inst = model.instruments (shown);
    juce::String q = inst.isEmpty() ? juce::String() : inst[0];
    search.setText (q, false);
    model.filter.query = q;
    model.refilter();
    if ((int) model.visible().size() <= 1) { search.setText ({}, false); model.filter.query.clear(); }
    filterChanged();
    showStatus ("Sounds like " + p.name + (model.filter.query.isNotEmpty() ? " (" + model.filter.query + ")" : juce::String()));
}

void MainPage::toggleFavorite (int idx)
{
    model.setFavorite (idx, ! model.isFavorite (idx));
    ctx.settings->setFavorites (model.favoriteIds());
    if (model.filter.favoritesOnly) filterChanged (false, true);   // the row goes away, the list stays where it is
    list.refresh();
    heartBtn.setToggleState (shown >= 0 && model.isFavorite (shown), juce::dontSendNotification);
}

void MainPage::giveFocusBack()
{
    // nobody in the plugin keeps the keyboard: the host gets the keys again (its computer-keyboard piano, shortcuts)
    ++focusReturnCount;
    search.giveAwayKeyboardFocus();
    unfocusAllComponents();
    repaint (searchRect().expanded (2));
}

void MainPage::updateToggles()
{
    recentBtn.setToggleState (model.filter.recentOnly, juce::dontSendNotification);
    favBtn.setToggleState (model.filter.favoritesOnly, juce::dontSendNotification);
}

void MainPage::showStatus (const juce::String& msg)
{
    statusMsg = msg;
    statusUntil = juce::Time::getMillisecondCounter() + 4000;
    repaint (curRect());
}

void MainPage::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // another plugin window changed the favorites: follow it
    auto fresh = ctx.settings->favorites();
    auto mine = model.favoriteIds();
    fresh.sort (false); mine.sort (false);
    if (fresh != mine)
    {
        model.setFavoriteIds (fresh);
        if (model.filter.favoritesOnly) filterChanged (false, true);
        list.refresh();
        heartBtn.setToggleState (shown >= 0 && model.isFavorite (shown), juce::dontSendNotification);
    }
    updateToggles();
}

juce::File MainPage::renderWavForDrag()
{
    const auto* p = ctx.proc.currentPresetData();
    if (p == nullptr || ctx.proc.isLoading()) return {};   // never decode samples on the UI thread
    float macros[vault::numMacros];
    for (int i = 0; i < vault::numMacros; ++i) macros[i] = ctx.proc.getMacro (i);
    constexpr double sr = 44100.0;
    const auto buf = VaultEngine::render (ctx.proc.currentSound(), vault::homeNote, 0.8f, 2.0, sr, macros);   // C5 (MIDI 60), with the edits
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
} // namespace vui
