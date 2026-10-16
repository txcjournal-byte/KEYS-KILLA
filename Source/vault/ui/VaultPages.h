#pragma once
#include "VaultControls.h"
#include "VaultWidgets.h"
#include "VaultSettings.h"
#include "../VaultProcessor.h"
#include <map>
#include <tuple>

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
juce::String shortName (const juce::String& target, const VaultPreset* sound = nullptr);   // "Cutoff", "Reverb Mix", "L2 Level"
juce::String longName (const juce::String& target, const VaultPreset* sound = nullptr);    // "Layer 2 Level", "Reverb Mix" (fx<n> = the slot's effect)
// the summed modulation of a target (LFOs x their depth + wheel / velocity / aftertouch) in knob units (-1..1)
float amountOn (const VaultPreset&, const juce::String& target, bool& any);
bool sameTarget (const VaultPreset&, const juce::String& a, const juce::String& b);   // "fx.reverb.mix" == "fx2.mix" (slot 2 = first reverb)
void show (Knob&, const VaultPreset&, const juce::String& target);           // the green ring of a knob
// the menu: LFO 1 / LFO 2 / Mod Wheel / Velocity / Aftertouch (add or remove), "Remove all modulation"
void showMenu (UiContext&, juce::Component& anchor, const juce::String& target);
// what a menu choice does (source 0 / 1 = LFO 1 / 2, 2 wheel, 3 velocity, 4 aftertouch: add or remove; -1 = remove all)
void apply (UiContext&, const juce::String& target, int source);
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
    void revertToPreset();                      // the explicit reset: every edit back (the "edited" badge's menu, settings)
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    juce::Rectangle<float> editedBadge() const { return editedR; }   // tests
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
    void updateQuickKnob (int q);               // position / text / tick of a MAIN knob from the effective value
    juce::Rectangle<int> screenRect() const, searchRect() const, curRect() const, catsRect() const, listRect() const, noticeRect() const,
                         macroPanelRect() const, leftRect() const, rightRect() const;
    juce::Rectangle<float> heartRect() const;

    VaultBrowserModel model;
    SearchField search;
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
    std::vector<std::unique_ptr<juce::ParameterAttachment>> quickAtts;

    int shown = -2, lastLoadState = -1, focusReturnCount = 0, ticks = 0;
    juce::Rectangle<float> editedR;
    bool editedHot = false, revealPending = false;
    static constexpr int countW = 104;           // the result count at the right end of the search field
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
    struct AddCard;
    int numCards() const;
    int numAddCards() const { return (int) addCards.size(); }
    void addLayer (int how);                        // 0 a copy of layer A, 1 an octave up, 2 an octave down
    void removeLastLayer();
    juce::Component* cardComponent (int i) const;   // tests
    struct MuteMemo { float level = 1.0f; std::vector<std::tuple<int, int, float, float>> targets; };   // macro, index, min, max
    std::map<juce::String, MuteMemo> muted;   // a layer switched off ("<preset id>:<layer>"): its level and level macro ranges

private:
    void rebuild();
    std::vector<std::unique_ptr<Card>> cards;
    std::vector<std::unique_ptr<AddCard>> addCards;
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
    void resetParam (int slot, int param);                   // double-click on a slot knob: the bank preset's value
    juce::String chainName() const;                          // the FX preset pill's text
    struct SlotKnob { Knob* knob; int slot, param; };
    std::vector<SlotKnob> slotKnobs() const;                 // the visible knobs of the slot cards (tests)

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
    std::vector<std::pair<int, juce::Rectangle<int>>> routeGroups;   // source, its caption area
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
    static const juce::StringArray& rateBases();          // "1/1" .. "1/64"
    static int rateIndex (int base, int feel);            // vault::divisions() index (feel 0 straight, 1 triplet, 2 dotted)
    static std::pair<int, int> baseAndFeel (int divisionIndex);   // base -1: a bar rate (no triplet / dotted)
    // the ARP RATE and GATE RATE knobs step through the same list (slow -> fast, per base dotted / straight / triplet), so
    // the same rate sits at the same angle on both and both label it the same way ("1/16T", "1/8D")
    static const std::vector<int>& rateKnobDivisions();
    static float rateKnobPos (int divisionIndex);         // 0..1
    static int rateFromKnob (float pos);                  // vault::divisions() index
    static juce::String rateKnobText (float pos);
    static const juce::StringArray& orderNames();         // VaultArp::Mode order
    static double exportBars (const VaultArp&);           // Drag MIDI length: whole bars, >= 2, one full pattern

private:
    void arpPatternMenu();
    void gatePatternMenu();
    void choiceMenu (Dropdown& d, const juce::StringArray& items, int current, std::function<void (int)> pick);
    PowerButton arpPower, gatePower;
    Dropdown arpPattern { "Pattern: Init" }, gatePattern { "Pattern: Init" }, latchDd { "Latch: Off" };
    Dropdown orderDd { "Order: Up" }, octModeDd { "Octaves: Up" }, velModeDd { "Velocity: Played x Steps" }, retrigDd { "Retrigger: New chord" };
    Pill arpKeep { "Keep while browsing", Pill::Icon::lock }, gateKeep { "Keep while browsing", Pill::Icon::lock };
    Segments feelSeg { { "Straight", "Triplet", "Dotted" } };
    Knob arpRate { "RATE", 40 }, arpOct { "OCTAVES", 40 }, arpGate { "GATE", 40 }, arpSwing { "SWING", 40 }, arpSteps { "STEPS", 40 };
    DragOutPill midiBtn { "Drag MIDI" };
    Knob gRate { "RATE", 40 }, gSteps { "STEPS", 40 }, gDepth { "DEPTH", 40 }, gSmooth { "SMOOTH", 40 }, gSwing { "SWING", 40 }, gMix { "MIX", 40 };
    std::unique_ptr<StepLane> velLane, lenLane, typeLane, slideLane, octLane, gateLane;
    juce::Rectangle<int> arpR, gateR;
};
} // namespace vui
