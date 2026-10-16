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
    tabs[pageLayers].setTooltip ("Every layer: level, pan, tuning, envelope, filter, voice");
    tabs[pageFx].setTooltip ("The effect chain of the sound (8 slots)");
    tabs[pageMod].setTooltip ("LFOs, mod wheel, velocity and aftertouch");
    tabs[pageArp].setTooltip ("Arpeggiator and gate");
    addAndMakeVisible (advBtn);
    advBtn.onClick = [this] { showPage (pageLayers); };
    advBtn.setTooltip (juce::String (juce::CharPointer_UTF8 ("Sound design pages: LAYERS, FX, MOD, ARP \xc2\xb7 GATE")));
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
    advBtn.setToggleState (page != pageMain, juce::dontSendNotification);
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
    x = sepX + 6;
    advBtn.setBounds (x, ty, advBtn.idealWidth(), 32);
    x += advBtn.idealWidth() + 2;
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
    // top bar: a brushed-metal faceplate (fine horizontal grain, a lit top edge, a bevelled lower edge)
    g.setGradientFill (ColourGradient (Colour (0xff1c2431), 0, 0, Colour (0xff121822), 0, (float) topH, false));
    g.fillRect (0.0f, 0.0f, b.getWidth(), (float) topH);
    {
        Random grain (17);
        for (int y = 2; y < topH - 2; y += 2)
        {
            g.setColour (Colours::white.withAlpha (0.006f + grain.nextFloat() * 0.012f));
            g.fillRect (0.0f, (float) y, b.getWidth(), 1.0f);
        }
    }
    g.setColour (Colour (0x14ffffff));
    g.fillRect (0.0f, 0.0f, b.getWidth(), 1.0f);
    g.setColour (Colour (0xff06080c));
    g.fillRect (0.0f, (float) topH - 1.0f, b.getWidth(), 1.0f);
    g.setGradientFill (ColourGradient (Colour (0x59000000), 0, (float) topH, Colour (0x00000000), 0, (float) topH + 8.0f, false));
    g.fillRect (0.0f, (float) topH, b.getWidth(), 8.0f);
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
    if (page != pageMain)
    {
        // the ADVANCED group: a thin bracket under the group header and its tabs
        const float x0 = (float) advBtn.getX() + 8.0f, x1 = (float) tabs[(size_t) numPages - 1].getRight() - 4.0f;
        g.setColour (col::ice2.withAlpha (0.35f));
        g.fillRect (x0, (float) advBtn.getBottom() + 3.0f, x1 - x0, 1.0f);
    }

    // keyboard bed: the dark cheek of an instrument - a lit edge, a recess the keys sit in
    const auto kr = Rectangle<float> (0.0f, (float) (getHeight() - keysH), b.getWidth(), (float) keysH);
    g.setGradientFill (ColourGradient (Colour (0xff161c26), 0, kr.getY(), Colour (0xff0a0d12), 0, kr.getBottom(), false));
    g.fillRect (kr);
    g.setColour (Colour (0xff06080c));
    g.fillRect (kr.getX(), kr.getY(), kr.getWidth(), 1.0f);
    g.setColour (Colour (0x12ffffff));
    g.fillRect (kr.getX(), kr.getY() + 1.0f, kr.getWidth(), 1.0f);
    const auto well = keyboard.getBounds().toFloat().expanded (3.0f, 3.0f).withTrimmedBottom (-1.0f);
    g.setColour (Colour (0xff040508));
    g.fillRoundedRectangle (well, 4.0f);
    g.setColour (Colour (0xff1e2633));
    g.drawRoundedRectangle (well.reduced (0.5f), 4.0f, 1.0f);
}

void VaultMainView::syncFromProcessor()
{
    mainP->syncFromProcessor();
    pageComponent (page).tick();
    {   // the on-screen keyboard shows what really sounds (the arp's notes, latched ones, the played keys)
        uint64_t m[2] {};
        proc.audioEngine().soundingNotes (m);
        keyboard.setSounding (m);
    }
    // ~10x per second: the sound / switches / locks changed elsewhere (host automation, state restore, a macro or MAIN knob
    // moved by the host): the pages follow. Nothing is repainted while nothing changed.
    if (++ticks % 3 == 0)
    {
        int sig = 0, bit = 1;
        for (auto* id : { VaultProcessor::idFxOn, VaultProcessor::idArpOn, VaultProcessor::idGateOn }) { sig |= proc.getSwitch (id) ? bit : 0; bit <<= 1; }
        for (auto sec : { VaultProcessor::Section::fx, VaultProcessor::Section::arp, VaultProcessor::Section::gate }) { sig |= proc.getLock (sec) ? bit : 0; bit <<= 1; }
        // the macros and the MAIN knobs move the effective values every knob shows (a macro sets the reverb mix ...)
        std::array<float, vault::numMacros + vault::numQuick> prm {};
        for (int m = 0; m < vault::numMacros; ++m) prm[(size_t) m] = proc.getMacro (m);
        for (int q = 0; q < vault::numQuick; ++q) prm[(size_t) (vault::numMacros + q)] = proc.getQuick (q);
        bool paramsMoved = false;
        for (size_t i = 0; i < prm.size(); ++i) paramsMoved |= std::abs (prm[i] - lastParams[i]) > 1.0e-6f;
        if (sig != lastStateSig || paramsMoved || ! (proc.currentSound() == lastSound)) { lastStateSig = sig; lastParams = prm; refreshAll(); }
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

std::vector<float> VaultMainView::scaleChoices() const
{
    // only the sizes whose window fits the screen the editor is on (never a window larger than the screen)
    const float cur = currentScale ? currentScale() : 1.0f;
    const auto* ed = findParentComponentOfClass<VaultEditor>();
    const auto area = ed != nullptr ? ed->displayArea() : juce::Rectangle<int>();
    std::vector<float> out;
    for (float sc : VaultEditor::scalePresets())
        if (area.isEmpty() || sc <= VaultEditor::minScale + 0.001f || VaultEditor::fits (sc, area) || std::abs (sc - cur) < 0.005f)
            out.push_back (sc);
    return out;
}

void VaultMainView::showScaleMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    const float cur = currentScale ? currentScale() : 1.0f;
    const auto choices = scaleChoices();
    for (int i = 0; i < (int) choices.size(); ++i)
        m.addItem (i + 1, juce::String (juce::roundToInt (choices[(size_t) i] * 100.0f)) + "%", true, std::abs (choices[(size_t) i] - cur) < 0.005f);
    juce::Component::SafePointer<VaultMainView> safe (this);
    // a desktop menu next to the button's screen area: JUCE keeps it inside the screen whatever the window's scale
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (scaleBtn.getScreenBounds()).withMinimumWidth (scaleBtn.getScreenBounds().getWidth())
                         .withMaximumNumColumns (1),
                     [safe, choices] (int r)
                     {
                         if (safe == nullptr || r <= 0 || r > (int) choices.size()) return;
                         if (safe->onScaleChosen) safe->onScaleChosen (choices[(size_t) r - 1]);
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
    m.addItem (3, "Revert the sound to the preset", proc.hasEdits());
    m.addSeparator();
    m.addItem (9, "Sound library: " + (proc.libraryFound() ? proc.libraryPath().getFullPathName() : juce::String ("not installed")), false);
    m.addItem (10, "Bank: " + proc.bankDescription(), false);
    juce::Component::SafePointer<VaultMainView> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gear), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 1) safe->settings->setPreviewOnClick (! safe->settings->previewOnClick());
        if (r == 2) { safe->proc.resetQuick(); safe->mainP->showStatus ("MAIN knobs back to the sound as designed"); }
        if (r == 3) safe->mainP->revertToPreset();
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

    // the keyboard: the editor takes it when the user clicks into the plugin (not on open), the computer keyboard then
    // plays notes; no child keeps it (the resize corner included), the search box only while it is being typed in
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (true);
    addMouseListener (this, true);
    keys.onOctaveChanged = [this]
    {
        if (main == nullptr) return;
        main->keys().setComputerKeys (keys.zNote(), keys.highestNote());
        main->showStatus ("Computer keys: Z = " + vault::noteName (keys.zNote()) + ", Q = " + vault::noteName (keys.zNote() + 12)
                          + juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 \xe2\x86\x90 \xe2\x86\x92 octave")));
    };
    main->keys().setComputerKeys (keys.zNote(), keys.highestNote());
    settingSize = true;   // the limits below resize the window once - don't remember that size
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (baseWidth * minScale), juce::roundToInt (baseHeight * minScale),
                     juce::roundToInt (baseWidth * maxScale), juce::roundToInt (baseHeight * maxScale));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setSize (juce::roundToInt (baseWidth * s), juce::roundToInt (baseHeight * s));
    limitToDisplay (area);
    settingSize = false;
    for (auto* c : getChildren())   // the resize corner: a click there must not move the keyboard anywhere
        if (c != main.get()) { c->setWantsKeyboardFocus (false); c->setMouseClickGrabsKeyboardFocus (false); }
    startTimerHz (30);
}

VaultEditor::~VaultEditor()
{
    keys.allOff();
    keys.onOctaveChanged = nullptr;
    stopTimer();
    removeMouseListener (this);
    main.reset();
    settings->flush();
    setLookAndFeel (nullptr);
}

void VaultEditor::setUiScale (float s)
{
    s = juce::jlimit (minScale, maxScale, s);
    if (const auto area = displayArea(); ! area.isEmpty() && ! fits (s, area)) s = restoredScale (s, area);   // never past the screen
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
    limitToDisplay (area);
}

void VaultEditor::limitToDisplay (juce::Rectangle<int> area)
{
    // the corner drag can never make the window larger than the screen it is on (aspect kept)
    if (area.isEmpty()) return;
    const float most = juce::jlimit (minScale, maxScale, std::min ((float) area.getWidth() / (float) baseWidth, (float) area.getHeight() / (float) baseHeight));
    const int w = juce::roundToInt (baseWidth * most), h = juce::roundToInt (baseHeight * most);
    if (auto* c = getConstrainer(); c != nullptr && (c->getMaximumWidth() != w || c->getMaximumHeight() != h))
    {
        const juce::ScopedValueSetter<bool> svs (settingSize, true);
        setResizeLimits (juce::roundToInt (baseWidth * minScale), juce::roundToInt (baseHeight * minScale), w, h);
    }
    lastArea = area;
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
    else if (displayChecked && ++displayTicks % 30 == 0 && isShowing())   // ~1x per second: moved to another screen?
        if (displayArea() != lastArea) fitToDisplay();
    // the keyboard comes back after a popup menu closed (or anything else left the plugin's window active with nothing
    // focused): once the user has clicked into the plugin, its active window always plays the computer keys
    if (clickedIn && displayTicks % 8 == 0 && isShowing() && ! main->searchHasFocus()
        && juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0)
        if (auto* peer = getPeer(); peer != nullptr && peer->isFocused())
            if (auto* f = juce::Component::getCurrentlyFocusedComponent(); f == nullptr || (f != this && ! isParentOf (f)))
                if (f == nullptr || f->getPeer() == peer) takeKeyboard();
}

void VaultEditor::mouseDown (const juce::MouseEvent& e)
{
    // a click anywhere but in the search box: the editor has the keyboard (typing in the search box ends, keys play notes)
    if (main == nullptr || e.eventComponent == nullptr) return;
    clickedIn = true;
    if (main->searchBox().isParentOf (e.eventComponent) || e.eventComponent == &main->searchBox()) return;
    if (main->searchHasFocus() || main->searchBox().getWantsKeyboardFocus()) main->giveFocusBack();   // (armed by a click, typed in)
    else if (juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0) takeKeyboard();   // (not while a menu is open)
}

void VaultEditor::takeKeyboard()
{
    if (isShowing() && ! hasKeyboardFocus (false)) grabKeyboardFocus();
}

bool VaultEditor::keyPressed (const juce::KeyPress& k)
{
    if (main == nullptr || main->searchHasFocus()) return false;   // typing in the search box
    if (keys.keyPressed (k)) return true;
    const auto mods = k.getModifiers();
    if (! mods.isCommandDown() && ! mods.isCtrlDown() && ! mods.isAltDown())
    {
        if (k.getKeyCode() == juce::KeyPress::upKey)   { main->step (-1, false); return true; }
        if (k.getKeyCode() == juce::KeyPress::downKey) { main->step (1, false); return true; }
    }
    return k.getKeyCode() == juce::KeyPress::tabKey;   // Tab never moves the keyboard into the search box
}

bool VaultEditor::keyStateChanged (bool)
{
    if (main == nullptr || main->searchHasFocus()) { keys.allOff(); return false; }
    return keys.keyStateChanged();
}

void VaultEditor::focusLost (FocusChangeType)
{
    keys.allOff();   // the search box got it, the window lost it: nothing may keep sounding
}

void VaultEditor::focusGained (FocusChangeType)
{
    if (main != nullptr && ! keysHintShown)
    {
        keysHintShown = true;
        main->showStatus (juce::String (juce::CharPointer_UTF8 ("Computer keys play notes: Z = C4, Q = C5 \xc2\xb7 \xe2\x86\x90 \xe2\x86\x92 octave \xc2\xb7 \xe2\x86\x91 \xe2\x86\x93 sound")));
    }
}
