#include "VaultEditor.h"
#include "ui/VaultMainView.h"

using namespace vui;

namespace
{
// remove any focus-on-click from a component tree (focus stays with the editor unless the search box is clicked)
void noClickFocus (juce::Component& c, const juce::Component* except)
{
    if (&c == except) return;
    c.setMouseClickGrabsKeyboardFocus (false);
    c.setWantsKeyboardFocus (false);
    for (auto* ch : c.getChildren()) noClickFocus (*ch, except);
}
const juce::Font logoFont() { return font (18.0f, true, 0.16f); }
const juce::Font advFont() { return font (14.0f, false, 0.16f); }
} // namespace

//==============================================================================
// VaultMainView
//==============================================================================
VaultMainView::VaultMainView (VaultProcessor& p, std::shared_ptr<VaultSettings> s)
    : proc (p), settings (std::move (s)), ctx (p, settings), keyboard (p.getKeyboardState())
{
    ctx.soundEdited = [this] { refreshAll(); };
    ctx.status = [this] (const juce::String& m) { mainP->showStatus (m); };
    mainP = std::make_unique<MainPage> (ctx);
    layersP = std::make_unique<LayersPage> (ctx);
    fxP = std::make_unique<FxPage> (ctx);
    modP = std::make_unique<ModPage> (ctx);
    arpP = std::make_unique<ArpGatePage> (ctx);
    for (int i = 0; i < numPages; ++i)
    {
        addChildComponent (pageComponent (i));
        addAndMakeVisible (tabs[(size_t) i]);
        tabs[(size_t) i].onClick = [this, i] { showPage (i); };
    }
    tabs[0].setTooltip ("Browse, play and tweak");
    for (auto* t : { &fxT, &arpT, &gateT }) addAndMakeVisible (*t);
    auto toggle = [this] (const char* id) { proc.setSwitch (id, ! proc.getSwitch (id)); refreshAll(); };
    fxT.onClick = [toggle] { toggle (VaultProcessor::idFxOn); };
    arpT.onClick = [toggle] { toggle (VaultProcessor::idArpOn); };
    gateT.onClick = [toggle] { toggle (VaultProcessor::idGateOn); };
    fxT.setTooltip ("Every effect of the sound on / off");
    arpT.setTooltip (juce::String (juce::CharPointer_UTF8 ("Arpeggiator on / off (settings: ARP \xc2\xb7 GATE)")));
    gateT.setTooltip (juce::String (juce::CharPointer_UTF8 ("Gate on / off (settings: ARP \xc2\xb7 GATE)")));
    for (auto* id : { VaultProcessor::idFxOn, VaultProcessor::idArpOn, VaultProcessor::idGateOn })
        if (auto* prm = proc.apvts.getParameter (id))
            switchAtts.push_back (std::make_unique<juce::ParameterAttachment> (*prm, [this] (float) { if (mainP != nullptr) refreshAll(); }, nullptr));
    addAndMakeVisible (scaleBtn);
    scaleBtn.onClick = [this] { showScaleMenu(); };
    scaleBtn.setTooltip ("Window size");
    addAndMakeVisible (gear);
    gear.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (keyboard);
    noClickFocus (*this, &mainP->searchBox());
    setWantsKeyboardFocus (false);
    showPage (pageMain);
    updateToggles();
}

VaultMainView::~VaultMainView()
{
    switchAtts.clear();
    ctx.soundEdited = nullptr;
    ctx.status = nullptr;
}

vui::Page& VaultMainView::pageComponent (int id)
{
    switch (id)
    {
        case pageLayers: return *layersP;
        case pageFx: return *fxP;
        case pageMod: return *modP;
        case pageArp: return *arpP;
        default: return *mainP;
    }
}

int VaultMainView::logoWidth()
{
    return (int) std::ceil (std::max (textWidth (logoFont(), "CHROME VAULT"), textWidth (font (14.0f, true, 0.3f), "BY TRAPWIN")));
}

juce::Rectangle<int> VaultMainView::pageArea() const
{
    return { padX, topH + padTop, getWidth() - 2 * padX, getHeight() - topH - keysH - padTop - padBottom };
}

void VaultMainView::showPage (int id)
{
    page = juce::jlimit (0, (int) numPages - 1, id);
    for (int i = 0; i < numPages; ++i)
    {
        pageComponent (i).setVisible (i == page);
        tabs[(size_t) i].setToggleState (i == page, juce::dontSendNotification);
    }
    pageComponent (page).refresh();
}

void VaultMainView::refreshAll()
{
    lastSound = proc.currentSound();
    pageComponent (page).refresh();
    if (page != pageMain) mainP->refresh();
    updateToggles();
}

void VaultMainView::resized()
{
    // top bar: logo | MAIN | ADVANCED  LAYERS FX MOD ARP · GATE ...... FX ARP GATE | 100% | gear
    int x = 16 + logoWidth() + 26;
    const int ty = (topH - 32) / 2;
    auto tab = [&] (int i) { auto& t = tabs[(size_t) i]; t.setBounds (x, ty, t.idealWidth(), 32); x += t.idealWidth() + 2; };
    tab (pageMain);
    sepX = x + 8;
    x = sepX + 10;
    advLabelX = x;
    x += (int) std::ceil (textWidth (advFont(), "ADVANCED")) + 8;
    for (int i = pageLayers; i < numPages; ++i) tab (i);
    int r = getWidth() - 16;
    const int by = (topH - 30) / 2;
    gear.setBounds (r - 32, by, 32, 30); r -= 32 + 8;
    const int sw = std::max (70, scaleBtn.idealWidth());
    scaleBtn.setBounds (r - sw, by, sw, 30); r -= sw + 8;
    for (auto* t : { &gateT, &arpT, &fxT }) { const int w = t->idealWidth(); t->setBounds (r - w, by, w, 30); r -= w + 8; }

    for (int i = 0; i < numPages; ++i) pageComponent (i).setBounds (pageArea());
    keyboard.setBounds (16, getHeight() - keysH + 6, getWidth() - 32, keysH - 14);
}

void VaultMainView::paint (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();
    {
        ColourGradient bg (col::winA, 0, 0, col::winC, 0, b.getBottom(), false);
        bg.addColour (0.6, col::winB);
        g.setGradientFill (bg);
        g.fillRect (b);
    }
    // top bar
    g.setGradientFill (ColourGradient (col::barA, 0, 0, col::barB, 0, (float) topH, false));
    g.fillRect (0.0f, 0.0f, b.getWidth(), (float) topH);
    g.setColour (col::line3);
    g.fillRect (0.0f, (float) topH - 1.0f, b.getWidth(), 1.0f);
    {   // logo
        g.setGradientFill (chromeGradient (8.0f, 30.0f));
        g.setFont (logoFont());
        g.drawText ("CHROME VAULT", 16, 7, logoWidth() + 10, 24, Justification::centredLeft, false);
        g.setColour (col::dim);
        g.setFont (font (14.0f, true, 0.3f));
        g.drawText ("BY TRAPWIN", 17, 30, logoWidth() + 10, 18, Justification::centredLeft, false);
    }
    g.setColour (col::line3);
    g.fillRect ((float) sepX, (float) topH * 0.5f - 9.0f, 1.0f, 18.0f);
    g.setFont (advFont());
    g.setColour (col::dim);
    g.drawText ("ADVANCED", advLabelX, 0, 120, topH, Justification::centredLeft, false);

    // keyboard bed
    const auto kr = Rectangle<float> (0.0f, (float) (getHeight() - keysH), b.getWidth(), (float) keysH);
    g.setColour (col::keyBed);
    g.fillRect (kr);
    g.setColour (col::line3);
    g.fillRect (kr.getX(), kr.getY(), kr.getWidth(), 1.0f);
}

void VaultMainView::syncFromProcessor()
{
    mainP->syncFromProcessor();
    pageComponent (page).tick();
    // ~10x per second: the sound / switches / locks changed elsewhere (host automation, state restore, a macro or MAIN knob
    // moved by the host): the pages follow. Nothing is repainted while nothing changed.
    if (++ticks % 3 == 0)
    {
        int sig = 0, bit = 1;
        for (auto* id : { VaultProcessor::idFxOn, VaultProcessor::idArpOn, VaultProcessor::idGateOn }) { sig |= proc.getSwitch (id) ? bit : 0; bit <<= 1; }
        for (auto sec : { VaultProcessor::Section::fx, VaultProcessor::Section::arp, VaultProcessor::Section::gate }) { sig |= proc.getLock (sec) ? bit : 0; bit <<= 1; }
        if (sig != lastStateSig || ! (proc.currentSound() == lastSound)) { lastStateSig = sig; refreshAll(); }
    }
}

void VaultMainView::updateToggles()
{
    fxT.setToggleState (proc.getSwitch (VaultProcessor::idFxOn), juce::dontSendNotification);
    arpT.setToggleState (proc.getSwitch (VaultProcessor::idArpOn), juce::dontSendNotification);
    gateT.setToggleState (proc.getSwitch (VaultProcessor::idGateOn), juce::dontSendNotification);
    bool relayout = false;
    for (auto [t, sec] : { std::pair<LedToggle*, VaultProcessor::Section> { &fxT, VaultProcessor::Section::fx },
                           { &arpT, VaultProcessor::Section::arp }, { &gateT, VaultProcessor::Section::gate } })
    {
        const bool l = proc.getLock (sec);
        if (l != t->locked) { t->locked = l; relayout = true; }
        t->setTooltip (juce::String (t->getButtonText()) + " on / off" + (l ? " (kept while browsing)" : ""));
        t->repaint();
    }
    const float s = currentScale ? currentScale() : 1.0f;
    const auto txt = juce::String (juce::roundToInt (s * 100.0f)) + "%";
    if (scaleBtn.getButtonText() != txt) { scaleBtn.setButtonText (txt); relayout = true; }
    if (relayout && getWidth() > 0) resized();
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
    for (float sc : VaultEditor::scalePresets())
    {
        const bool ticked = std::abs (sc - cur) < 0.005f;
        const bool ok = area.isEmpty() || ticked || sc <= VaultEditor::minScale || VaultEditor::fits (sc, area);
        m.addItem (id++, juce::String (juce::roundToInt (sc * 100.0f)) + "%", ok, ticked);
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

void VaultMainView::showSettingsMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addItem (1, "Preview a sound when it is clicked (C5)", true, settings->previewOnClick());
    m.addSeparator();
    m.addItem (2, "Reset the MAIN knobs");
    m.addItem (3, "Revert the sound to the preset", proc.isSoundEdited());
    m.addSeparator();
    m.addItem (9, "Sound library: " + (proc.libraryFound() ? proc.libraryPath().getFullPathName() : juce::String ("not installed")), false);
    m.addItem (10, "Bank: " + proc.bankDescription(), false);
    juce::Component::SafePointer<VaultMainView> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gear), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 1) safe->settings->setPreviewOnClick (! safe->settings->previewOnClick());
        if (r == 2) { safe->proc.resetQuick(); safe->mainP->showStatus ("MAIN knobs back to the sound as designed"); }
        if (r == 3) { safe->proc.revertSound(); safe->mainP->showStatus ("Sound back to the preset"); }
        safe->refreshAll();
    });
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
