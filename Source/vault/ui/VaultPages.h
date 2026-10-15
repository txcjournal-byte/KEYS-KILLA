#pragma once
#include "VaultControls.h"
#include "VaultWidgets.h"
#include "VaultSettings.h"
#include "../VaultProcessor.h"
#include <map>

// CHROME VAULT v3 editor pages (the approved v3 mockup): MAIN (browser screen + quick knobs + sound macros) and the
// ADVANCED pages LAYERS, FX, MOD, ARP · GATE. Every page reads the processor's current sound (VaultProcessor::currentSound)
// and changes it with VaultProcessor::editSound; the host parameters (macros, MAIN knobs, switches) go through attachments.
// Layout: each page is laid out at the base size (1148 x 612 inside the 1180 x 760 window) and scaled with the window.
namespace vui
{
//==============================================================================
// shared by the pages
struct UiContext
{
    UiContext (VaultProcessor& p, std::shared_ptr<VaultSettings> s) : proc (p), settings (std::move (s)) {}
    VaultProcessor& proc;
    std::shared_ptr<VaultSettings> settings;
    std::function<void()> soundEdited;                       // after an edit: the pages refresh
    std::function<void (const juce::String&)> status;        // a short message (the MAIN page's info line)

    const VaultPreset& sound() const { return proc.currentSound(); }
    // change the current sound (an undoable host edit is not needed: the sound is saved with the project)
    void edit (const std::function<void (VaultPreset&)>& f) { proc.editSound (f); if (soundEdited) soundEdited(); }
    // the current preset as it is in the bank (v3): what a double-click returns a knob to
    const VaultPreset& bankSound()
    {
        const int i = proc.currentPreset();
        if (i != cachedFor && juce::isPositiveAndBelow (i, (int) proc.bank().size()))
        {
            cached = proc.bank()[(size_t) i];
            vault::upgradeToV3 (cached);
            cachedFor = i;
        }
        return cached;
    }
    void say (const juce::String& s) { if (status) status (s); }

private:
    int cachedFor = -1;
    VaultPreset cached;
};

//==============================================================================
// modulation (right-click any knob -> "Modulate with..."; the knob shows a green ring)
namespace mod
{
float range (const juce::String& target);                                      // target units of one full knob turn
juce::String amountText (const juce::String& target, float amount);          // "8 ct", "+0.5 oct", "25%"
juce::String shortName (const juce::String& target);                          // "Cutoff", "Reverb Mix", "L2 Level"
// the summed modulation of a target (LFOs x their depth + wheel / velocity / aftertouch) in knob units (-1..1)
float amountOn (const VaultPreset&, const juce::String& target, bool& any);
void show (Knob&, const VaultPreset&, const juce::String& target);           // the green ring of a knob
// the menu: LFO 1 / LFO 2 / Mod Wheel / Velocity / Aftertouch (add or remove), "Remove all modulation"
void showMenu (UiContext&, juce::Component& anchor, const juce::String& target);
// the targets offered by "+ Add" (grouped submenus)
void addTargetsMenu (juce::PopupMenu&, const VaultPreset&, int firstId, juce::StringArray& idsOut);
} // namespace mod

// text helpers
juce::String pct (float v01);                    // "35%"
juce::String signedPct (float v);                // "+35%" / "-10%"
juce::String secondsText (float s);              // "4 ms" / "1.2 s"
juce::String hzText (float hz);                  // "8.2 kHz" / "440 Hz"
juce::String choiceName (const vault::FxParamInfo&, float v);   // a choice / division name, else ""
juce::String fxValueText (const vault::FxParamInfo&, float v);  // any FX parameter for a knob

//==============================================================================
class Page : public juce::Component
{
public:
    explicit Page (UiContext& c) : ctx (c) { noFocus (*this); }
    virtual void refresh() = 0;   // read the current sound / parameters (cheap: called ~10x per second while shown)
    virtual void tick() {}        // 30 Hz while shown (step lights, LFO phase)

protected:
    UiContext& ctx;
};

//==============================================================================
// MAIN: the browser "screen" (search, categories with counts, the list, the current sound), FILTER · AMP + MOTION
// knobs on the left, EFFECTS (LED = on / off) + OUTPUT on the right, the 4 named sound macros under the screen
class MainPage : public Page, private juce::ChangeListener
{
public:
    MainPage (UiContext&);
    ~MainPage() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    void tick() override;

    // browsing (message thread)
    void loadPreset (int bankIndex, bool audition);
    void step (int delta, bool wrap);
    void auditionCurrent();
    void syncFromProcessor();
    void setCategory (int c);
    void setQuery (const juce::String& q);
    void setLayers (int n);
    void setFavoritesOnly (bool);
    void setRecentOnly (bool);
    void toggleFavorite (int bankIndex);
    void moreLikeThis();
    void showStatus (const juce::String& msg);
    int visibleCount() const { return (int) model.visible().size(); }
    int shownPreset() const { return shown; }
    bool searchHasFocus() const { return search.hasKeyboardFocus (true); }
    juce::TextEditor& searchBox() { return search; }
    VaultBrowserModel& browser() { return model; }
    PresetList& presetList() { return list; }
    CategoryList& categoryList() { return cats; }
    juce::File renderWavForDrag();
    juce::String statusHint() const;
    void giveFocusBack();
    int focusReturns() const { return focusReturnCount; }

    // the shown sound's state: samples loading, library not installed, an instrument the installed library lacks
    enum class Notice { none, loading, libraryMissing, instrumentMissing };
    Notice notice() const;
    enum class PartState { ready, loading, missing };
    PartState partState (const VaultPart&) const;
    juce::String noticeTitle() const;
    juce::String composition() const;           // "Piano · Concert Grand + Air Pad + Tape Room"

    Knob& quickKnob (int q) { return quick[(size_t) q]; }
    Knob& macroKnob (int i) { return macro[(size_t) i]; }
    Knob& volumeKnob() { return volume; }
    static juce::String quickTarget (int q);    // the modulation target a MAIN knob stands for

private:
    struct SearchKeys : juce::KeyListener
    {
        MainPage& v;
        explicit SearchKeys (MainPage& o) : v (o) {}
        bool keyPressed (const juce::KeyPress& k, juce::Component*) override
        {
            if (k == juce::KeyPress::downKey) { v.step (1, false); return true; }
            if (k == juce::KeyPress::upKey)   { v.step (-1, false); return true; }
            return false;
        }
    } searchKeys { *this };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void filterChanged (bool scrollToCurrent = true, bool keepScroll = false);
    void setShown (int bankIndex);
    void updateToggles();
    juce::String quickText (int q, float v) const;
    juce::Rectangle<int> screenRect() const, searchRect() const, curRect() const, catsRect() const, listRect() const, noticeRect() const,
                         macroPanelRect() const, leftRect() const, rightRect() const;
    juce::Rectangle<float> heartRect() const;

    VaultBrowserModel model;
    juce::TextEditor search;
    ArrowButton prevBtn { false }, nextBtn { true };
    Pill favBtn { "Favorites", Pill::Icon::heart }, recentBtn { "Recent" }, moreBtn { "More like this" };
    DragOutPill wavBtn { "Drag WAV" };
    Pill heartBtn { "", Pill::Icon::heart };
    CategoryList cats;
    PresetList list;
    std::array<Knob, vault::numQuick> quick;
    std::array<Knob, vault::numMacros> macro;
    Knob volume { "VOLUME" };
    std::vector<std::unique_ptr<KnobAttachment>> atts;

    int shown = -2, lastLoadState = -1, focusReturnCount = 0, ticks = 0;
    juce::String statusMsg;
    juce::uint32 statusUntil = 0;
};

//==============================================================================
class LayersPage : public Page
{
public:
    explicit LayersPage (UiContext&);
    ~LayersPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    struct Card;
    int numCards() const;
    std::map<juce::String, float> muted;   // the level of a layer switched off ("<preset id>:<layer>")

private:
    void rebuild();
    std::vector<std::unique_ptr<Card>> cards;
    juce::String layoutKey;
    Segments voiceMode { { "POLY", "MONO", "LEGATO" } };
    std::array<Knob, 5> voice;   // glide, unison, detune, vel sens, bend
    Pill resetBtn { "Reset to preset" };
    juce::Rectangle<int> voiceR;
};

//==============================================================================
class FxPage : public Page
{
public:
    explicit FxPage (UiContext&);
    ~FxPage() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    struct SlotCard;
    void beginDrag (int slot);
    void dragTo (juce::Point<int> posInPage);
    void endDrag();
    int slotAt (juce::Point<int> posInPage) const;
    juce::Rectangle<int> cardRect (int i) const;
    static std::array<int, 3> mainParams (vault::FxType);   // the 3 knobs of a slot card (-1 = none)
    static juce::String variantText (const VaultFxSlot&);   // "Hall", "Tape", "Ping-pong", "" ...
    void showSlotMenu (int slot, juce::Component& anchor);
    void showPresetMenu();
    juce::String chainName() const;                          // the FX preset pill's text

private:
    std::vector<std::unique_ptr<SlotCard>> slots;
    Dropdown presetBtn;
    Pill bypassBtn { "Bypass all" }, keepBtn { "Keep while browsing", Pill::Icon::lock };
    int dragFrom = -1, dragOver = -1;
    juce::String appliedChain;                               // the FX preset picked from the menu (until the chain changes)
};

//==============================================================================
class ModPage : public Page
{
public:
    explicit ModPage (UiContext&);
    ~ModPage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    void tick() override;
    struct LfoCard;
    struct Chip;

private:
    void rebuildRoutes();
    std::array<std::unique_ptr<LfoCard>, vault::numLfos> lfos;
    std::vector<std::unique_ptr<Knob>> routeKnobs;
    std::vector<std::pair<int, int>> routeIds;            // source (0 wheel, 1 velocity, 2 aftertouch), index
    Pill addWheel { "Wheel", Pill::Icon::plus }, addVel { "Velocity", Pill::Icon::plus }, addAt { "Aftertouch", Pill::Icon::plus };
    juce::String routeKey;
    juce::Rectangle<int> routesR;
};

//==============================================================================
class StepLane;
class ArpGatePage : public Page
{
public:
    explicit ArpGatePage (UiContext&);
    ~ArpGatePage() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh() override;
    void tick() override;
    static const juce::StringArray& rateBases();          // "1/1" .. "1/32"
    static int rateIndex (int base, int feel);            // vault::divisions() index (feel 0 straight, 1 triplet, 2 dotted)
    static std::pair<int, int> baseAndFeel (int divisionIndex);

private:
    void arpPatternMenu();
    void gatePatternMenu();
    PowerButton arpPower, gatePower;
    Dropdown arpPattern { "Pattern: Init" }, gatePattern { "Pattern: Init" };
    Pill latchBtn { "Latch" }, arpKeep { "Keep while browsing", Pill::Icon::lock }, gateKeep { "Keep while browsing", Pill::Icon::lock };
    Segments modeSeg { { "Up", "Down", "Up/Down", "As Played", "Random", "Chord" } }, feelSeg { { "Straight", "Triplet", "Dotted" } };
    Knob arpRate { "RATE", 40 }, arpOct { "OCTAVES", 40 }, arpGate { "GATE", 40 }, arpSwing { "SWING", 40 }, arpSteps { "STEPS", 40 };
    DragOutPill midiBtn { "Drag MIDI" };
    Knob gRate { "RATE", 40 }, gDepth { "DEPTH", 40 }, gSmooth { "SMOOTH", 40 }, gSwing { "SWING", 40 }, gMix { "MIX", 40 };
    std::unique_ptr<StepLane> velLane, typeLane, octLane, gateLane;
    juce::Rectangle<int> arpR, gateR;
};
} // namespace vui
