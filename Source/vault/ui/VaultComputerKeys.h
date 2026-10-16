#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

// CHROME VAULT: the computer keyboard plays notes while the plugin window has the keyboard (the user clicked into it and
// is not typing in the search box). The layout is FL Studio's typing keyboard:
//   lower row  Z S X D C V G B H N J M  ,  L  .  ;  /    = C4 .. E5   (Z = C4 = MIDI 48)
//   upper row  Q 2 W 3 E R 5 T 6 Y 7 U  I  9  O  0  P  [ = ]   = C5 .. G6   (Q = C5 = MIDI 60, the home note)
// Left / Right arrow: octave down / up (-3 .. +3). Fixed velocity 100. A note-off comes when its key goes up (polled with
// KeyPress::isKeyCurrentlyDown on every key event, like juce::MidiKeyboardComponent), every note stops when the editor
// loses the keyboard. The notes go into the processor's MidiKeyboardState: they play and light the on-screen keyboard.
namespace vui
{
class ComputerKeys
{
public:
    static constexpr int velocity = 100;
    static constexpr int lowestC = 48;                       // Z at octave shift 0
    static constexpr int minShift = -3, maxShift = 3;

    explicit ComputerKeys (juce::MidiKeyboardState& s) : state (s) {}
    ~ComputerKeys() { allOff(); }

    // semitones above the Z note for a key code (lower case), -1 = not a note key
    static int offsetOf (int keyCode)
    {
        static const std::pair<char, int> map[] {
            { 'z', 0 }, { 's', 1 }, { 'x', 2 }, { 'd', 3 }, { 'c', 4 }, { 'v', 5 }, { 'g', 6 }, { 'b', 7 }, { 'h', 8 }, { 'n', 9 },
            { 'j', 10 }, { 'm', 11 }, { ',', 12 }, { 'l', 13 }, { '.', 14 }, { ';', 15 }, { '/', 16 },
            { 'q', 12 }, { '2', 13 }, { 'w', 14 }, { '3', 15 }, { 'e', 16 }, { 'r', 17 }, { '5', 18 }, { 't', 19 }, { '6', 20 },
            { 'y', 21 }, { '7', 22 }, { 'u', 23 }, { 'i', 24 }, { '9', 25 }, { 'o', 26 }, { '0', 27 }, { 'p', 28 }, { '[', 29 },
            { '=', 30 }, { ']', 31 } };
        const auto k = (int) juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) keyCode);
        for (auto& [c, o] : map) if (k == (int) c) return o;
        return -1;
    }
    static const juce::String& keyChars() { static const juce::String s ("zsxdcvgbhnjm,l.;/q2w3er5t6y7ui9o0p[=]"); return s; }

    int noteFor (int keyCode) const
    {
        const int o = offsetOf (keyCode);
        if (o < 0) return -1;
        const int n = lowestC + 12 * shift + o;
        return juce::isPositiveAndBelow (n, 128) ? n : -1;
    }
    int octaveShift() const { return shift; }
    int zNote() const { return lowestC + 12 * shift; }       // the lowest note the keys play (Z)
    int highestNote() const { return zNote() + 31; }        // ']'
    std::function<void()> onOctaveChanged;
    std::function<bool (int keyCode)> isKeyDown = [] (int k) { return juce::KeyPress::isKeyCurrentlyDown (k); };   // tests replace it

    // a key press on the editor: true = used (a note key or an octave key)
    bool keyPressed (const juce::KeyPress& k)
    {
        const auto mods = k.getModifiers();
        if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown()) return false;
        if (k.getKeyCode() == juce::KeyPress::leftKey)  { setShift (shift - 1); return true; }
        if (k.getKeyCode() == juce::KeyPress::rightKey) { setShift (shift + 1); return true; }
        const int code = (int) juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k.getKeyCode());
        const int note = noteFor (code);
        if (note < 0) return false;
        press (code, note);   // (usually already down: keyStateChanged saw it first)
        return true;
    }
    // any key went down / up: start what is down, stop what is up. True when a note key was involved.
    bool keyStateChanged()
    {
        bool any = false;
        for (size_t i = held.size(); i-- > 0;)
            if (! isKeyDown (held[i].key))
            {
                state.noteOff (1, held[i].note, 0.0f);
                held.erase (held.begin() + (std::ptrdiff_t) i);
                any = true;
            }
        if (juce::ModifierKeys::currentModifiers.isCommandDown() || juce::ModifierKeys::currentModifiers.isCtrlDown()
            || juce::ModifierKeys::currentModifiers.isAltDown())
            return any;
        for (auto ch : keyChars())
        {
            const int code = (int) ch;
            if (isHeld (code) || ! isKeyDown (code)) continue;
            if (const int note = noteFor (code); note >= 0) { press (code, note); any = true; }
        }
        return any;
    }
    void allOff()
    {
        for (auto& h : held) state.noteOff (1, h.note, 0.0f);
        held.clear();
    }
    int numHeld() const { return (int) held.size(); }
    void setShift (int s)
    {
        s = juce::jlimit ((int) minShift, (int) maxShift, s);
        if (s == shift) return;
        allOff();   // nothing may hang in the old octave
        shift = s;
        if (onOctaveChanged) onOctaveChanged();
    }

private:
    struct Held { int key, note; };
    bool isHeld (int code) const { for (auto& h : held) if (h.key == code) return true; return false; }
    void press (int code, int note)
    {
        if (isHeld (code)) return;
        for (auto& h : held) if (h.note == note) return;   // ',' and 'q' are the same C: one note
        held.push_back ({ code, note });
        state.noteOn (1, note, (float) velocity / 127.0f);
    }

    juce::MidiKeyboardState& state;
    std::vector<Held> held;
    int shift = 0;
};
} // namespace vui
