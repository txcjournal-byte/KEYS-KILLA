#pragma once
#include "VaultTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>

// CHROME VAULT v3 knob: a compact machined knob - dark anodised cap with a fine bevel and a soft shadow, a fine scale
// around it, a thin ice value ring and a crisp pointer; the LABEL (small caps) under it and the value under that (always on
// the MAIN page, on hover / drag elsewhere). A modulated knob shows a green arc over the scale (amount of the modulation),
// its value ring turns green. A small ice tick on the scale marks "as designed" where the owner sets one (markValue).
//   drag up / right = more (shift = fine), mouse wheel steps, double-click = the preset's value, right-click = menu
//   (the "Modulate with..." menu where a target exists). An optional LED in front of the label switches the effect on / off
//   (click the label).
// The knob never takes the keyboard focus. Value 0..1 (callers map it to the real range).
namespace vui
{
struct KnobLook
{
    float value = 0, arcOrigin = 0, modAmount = 0, mark = -1;   // mark < 0: none
    bool enabled = true, hot = false, modulated = false, inactive = false;
};

// a small LED (on = green with a soft glow)
inline void drawLed (juce::Graphics& g, juce::Rectangle<float> led, bool on, juce::Colour onColour = col::ok)
{
    using juce::Colour;
    if (on)
    {
        g.setColour (onColour.withAlpha (0.22f)); g.fillEllipse (led.expanded (2.5f));
        g.setGradientFill (juce::ColourGradient (onColour.brighter (0.5f), led.getCentreX(), led.getY(), onColour.darker (0.2f), led.getCentreX(), led.getBottom(), false));
        g.fillEllipse (led);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (Colour (0xff0b0f15), led.getCentreX(), led.getY(), Colour (0xff2a3446), led.getCentreX(), led.getBottom(), false));
        g.fillEllipse (led);
        g.setColour (Colour (0xff3a465c));
        g.drawEllipse (led.reduced (0.25f), 0.5f);
    }
}

inline void drawKnobBody (juce::Graphics& g, juce::Rectangle<float> box, const KnobLook& k)
{
    // style A "Machined" (docs/qa/design, knob-options.html function A): the reference is drawn at 58 px; every offset
    // scales with the knob (u = size / 58), the value ring stays a crisp 2.5 px (2.2 px on small knobs)
    using namespace juce;
    const float s = std::min (box.getWidth(), box.getHeight());
    const auto c = box.getCentre();
    const float R = s * 0.5f, u = s / 58.0f;
    const float a0 = MathConstants<float>::pi * -0.75f, a1 = MathConstants<float>::pi * 0.75f;
    auto angleOf = [&] (float v) { return a0 + jlimit (0.0f, 1.0f, v) * (a1 - a0); };
    const float ang = angleOf (k.value);
    const float dim = k.enabled ? 1.0f : 0.55f;

    // scale: 11 ticks (every 5th longer), #3A4558
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (float) i / 10.0f * (a1 - a0);
        const bool major = i % 5 == 0;
        const float r1 = R - 2.0f * u, r2 = R - (major ? 7.0f : 5.0f) * u;
        g.setColour (Colour (major ? 0xff4a5670 : 0xff3a4558).withMultipliedAlpha (dim));
        g.drawLine ({ c.getPointOnCircumference (r2, a), c.getPointOnCircumference (r1, a) }, major ? 1.3f : 1.1f);
    }
    // value ring: a dark track + the ice value arc (green when modulated, grey when its effect is off)
    const float rr = R - 10.0f * u, rw = s >= 40.0f ? 2.5f : 2.2f;
    {
        Path track; track.addCentredArc (c.x, c.y, rr, rr, 0.0f, a0, a1, true);
        g.setColour (Colour (0xff1b2330).withMultipliedAlpha (k.enabled ? 1.0f : 0.8f));
        g.strokePath (track, PathStrokeType (rw, PathStrokeType::curved, PathStrokeType::rounded));
    }
    const float ao = angleOf (k.arcOrigin);
    if (std::abs (ang - ao) > 0.01f)
    {
        Path v; v.addCentredArc (c.x, c.y, rr, rr, 0.0f, std::min (ao, ang), std::max (ao, ang), true);
        const auto vc = ! k.enabled || k.inactive ? Colour (0xff566277) : k.modulated ? col::ok : col::ice;
        if (k.enabled && ! k.inactive && k.hot)
        {
            g.setColour (vc.withAlpha (0.18f));
            g.strokePath (v, PathStrokeType (rw + 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (vc);
        g.strokePath (v, PathStrokeType (rw, PathStrokeType::curved, PathStrokeType::rounded));
    }
    if (k.arcOrigin > 0.001f && k.arcOrigin < 0.999f)
    {
        // bipolar: the centre of the scale is marked
        g.setColour (Colour (0xff6c7890).withMultipliedAlpha (dim));
        g.drawLine ({ c.getPointOnCircumference (R - 7.0f * u, ao), c.getPointOnCircumference (R - 1.5f * u, ao) }, 1.3f);
    }
    if (k.mark >= 0.0f && k.enabled)
    {
        // "as designed": a small ice tick on the scale
        const float am = angleOf (k.mark);
        g.setColour (col::ice2.withAlpha (0.95f));
        g.drawLine ({ c.getPointOnCircumference (R - 7.0f * u, am), c.getPointOnCircumference (R - 1.0f * u, am) }, 1.6f);
    }
    if (std::abs (k.modAmount) > 0.004f && k.enabled)
    {
        // the modulation range: a thin green arc over the scale, from the value in the direction of the amount
        const float rm = R - 3.5f * u;
        const float to = jlimit (a0, a1, ang + k.modAmount * (a1 - a0));
        Path m; m.addCentredArc (c.x, c.y, rm, rm, 0.0f, std::min (ang, to), std::max (ang, to), true);
        g.setColour (col::ok.withAlpha (0.95f));
        g.strokePath (m, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // the cap: soft drop shadow, a bevelled edge (#7C889C -> #0D1118, top to bottom), the machined face
    // (radial #5A6578 -> #2B3341 -> #151A22 from the upper left)
    const float re = R - 14.0f * u, rf = R - 15.5f * u;
    g.setColour (Colour (0xb305070a));
    g.fillEllipse (Rectangle<float> (re * 2.0f, re * 2.0f).withCentre (c.translated (0.0f, 1.5f * u + 0.5f)));
    g.setColour (Colour (0x30000000));
    g.fillEllipse (Rectangle<float> (re * 2.0f + 2.0f, re * 2.0f + 2.0f).withCentre (c.translated (0.0f, 2.5f * u + 0.5f)));
    {
        const auto edge = Rectangle<float> (re * 2.0f, re * 2.0f).withCentre (c);
        ColourGradient eg (Colour (k.hot && k.enabled ? 0xff97a3b8 : 0xff7c889c).withMultipliedAlpha (k.enabled ? 1.0f : 0.75f), c.x, edge.getY(),
                           Colour (0xff0d1118), c.x, edge.getBottom(), false);
        g.setGradientFill (eg);
        g.fillEllipse (edge);
    }
    const auto face = Rectangle<float> (rf * 2.0f, rf * 2.0f).withCentre (c);
    {
        const Point<float> hl (face.getX() + face.getWidth() * 0.45f, face.getY() + face.getHeight() * 0.35f);
        ColourGradient fg (Colour (k.enabled ? 0xff5a6578 : 0xff3c4554), hl.x, hl.y,
                           Colour (0xff151a22), hl.x + face.getWidth() * 0.75f, hl.y, true);
        fg.addColour (0.45, Colour (k.enabled ? 0xff2b3341 : 0xff222833));
        g.setGradientFill (fg);
        g.fillEllipse (face);
        // fine machining: faint concentric rings
        g.setColour (Colours::white.withAlpha (0.03f));
        for (float f : { 0.78f, 0.56f })
            g.drawEllipse (Rectangle<float> (face.getWidth() * f, face.getHeight() * f).withCentre (c), 0.6f);
    }
    // pointer: a white line from near the centre to the face edge (#F2F5FA, round ends)
    {
        const float r0 = std::max (0.0f, (R - 26.0f * u) * 0.15f), r1 = R - 17.0f * u;
        const auto p0 = c.getPointOnCircumference (r0, ang), p1 = c.getPointOnCircumference (std::max (r0 + 2.0f, r1), ang);
        const float pw = s >= 40.0f ? 2.0f : 1.8f;
        g.setColour (Colour (0x80000000));
        Path sh; sh.startNewSubPath (p0.translated (0.0f, 0.8f)); sh.lineTo (p1.translated (0.0f, 0.8f));
        g.strokePath (sh, PathStrokeType (pw + 0.6f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (! k.enabled ? Colour (0xff6a7588) : k.inactive ? Colour (0xffaab3c2) : Colour (0xfff2f5fa));
        Path ptr; ptr.startNewSubPath (p0); ptr.lineTo (p1);
        g.strokePath (ptr, PathStrokeType (pw, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Knob (const juce::String& labelText = {}, int knobDiameter = 44) : label (labelText), diameter (knobDiameter)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setRepaintsOnMouseActivity (true);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    // look
    juce::String label, shortLabel;          // shortLabel: used when the label does not fit the knob's width
    int diameter;
    float arcOrigin = 0.0f;                  // where the value arc starts (0.5 = bipolar)
    float modAmount = 0.0f;                  // -1..1 of the range: the green modulation ring
    float markValue = -1.0f;                 // >= 0: a small tick on the scale ("as designed")
    bool valueAlways = false;                // the value line always shows (MAIN page); else on hover / drag
    bool inactive = false;                   // shown, but switched off (an effect that is off): the value ring is grey
    bool hasLed = false, ledOn = false;
    int steps = 0;                           // > 1: snaps to steps (integer / choice parameters)
    float sensitivity = 1.0f;                // < 1 = slower drag
    std::function<juce::String (float)> valueText;   // the value line (default: percent)

    // behaviour
    float resetValue = 0.5f;                 // double-click target (the preset's value)
    std::function<void()> onReset;           // double-click: when set, called instead of moving to resetValue
    std::function<void (float)> onChange;    // the user moved it (value 0..1)
    std::function<void()> onGestureStart, onGestureEnd;
    std::function<void()> onLedClick;
    std::function<void (const juce::MouseEvent&)> onRightClick;

    static constexpr int labelH = 18, valueH = 17, gap = 3;
    static int heightFor (int d) { return d + gap + labelH + valueH; }
    int idealHeight() const { return heightFor (diameter); }

    float getValue() const { return value; }
    // from the outside (host, preset): ignored while the user drags it (no fighting)
    void setValue (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (dragging || std::abs (v - value) < 1.0e-6f) return;
        value = v;
        repaint();
    }
    void setEnabledLook (bool e) { if (e != enabledLook) { enabledLook = e; setMouseCursor (e ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor); repaint(); } }
    bool isEnabledLook() const { return enabledLook; }
    void setModulated (bool m, float amount) { if (m != modulated || std::abs (amount - modAmount) > 1e-4f) { modulated = m; modAmount = amount; repaint(); } }
    bool isModulated() const { return modulated; }
    bool isDragging() const { return dragging; }
    juce::String currentValueText() const { return valueText ? valueText (value) : juce::String (juce::roundToInt (value * 100.0f)) + "%"; }

    // tests: drive it like a user
    void userSet (float v) { begin(); apply (v); end(); }

    juce::Rectangle<float> knobArea() const
    {
        const float d = (float) diameter;
        return { ((float) getWidth() - d) * 0.5f, 0.0f, d, d };
    }
    juce::Rectangle<int> labelArea() const { return { 0, diameter + gap, getWidth(), labelH }; }

    void paint (juce::Graphics& g) override
    {
        const bool hot = isMouseOverOrDragging();
        KnobLook look;
        look.value = value; look.arcOrigin = arcOrigin; look.modAmount = modulated ? modAmount : 0.0f; look.mark = markValue;
        look.enabled = enabledLook; look.hot = hot; look.modulated = modulated; look.inactive = inactive;
        drawKnobBody (g, knobArea(), look);
        // label (with the LED in front of it)
        auto lr = labelArea().toFloat();
        auto f = font (14.0f, true, 0.12f);
        juce::String txt = label;
        const float ledW = hasLed ? 11.0f : 0.0f;
        if (textWidth (f, txt) + ledW > lr.getWidth()) f = font (14.0f, true, 0.04f);
        if (textWidth (f, txt) + ledW > lr.getWidth()) f = font (14.0f, true, 0.0f);
        if (textWidth (f, txt) + ledW > lr.getWidth() && shortLabel.isNotEmpty()) txt = shortLabel;
        while (textWidth (f, txt) + ledW > lr.getWidth() && txt.length() > 3) txt = txt.dropLastCharacters (1);   // never overlaps a neighbour
        const float tw = textWidth (f, txt) + ledW;
        float x = lr.getCentreX() - tw * 0.5f;
        if (hasLed)
        {
            drawLed (g, juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ x + 3.0f, lr.getCentreY() }), ledOn);
            x += ledW;
        }
        g.setFont (f);
        g.setColour (enabledLook ? (hasLed && ! ledOn ? col::dim : col::muted) : col::dim);
        g.drawText (txt, juce::Rectangle<float> (x, lr.getY(), tw - ledW + 4.0f, lr.getHeight()), juce::Justification::centredLeft, false);
        // value: always on the MAIN page, elsewhere while the mouse is on the knob
        if (valueAlways || (hot && enabledLook))
        {
            g.setFont (font (14.0f));
            g.setColour (hot && enabledLook ? col::ice : col::dim);
            // never clipped: a long value ("+1.20 oct", "14% Plate") is narrowed a little to the knob's width
            g.drawFittedText (currentValueText(), juce::Rectangle<int> (0, (int) lr.getBottom(), getWidth(), valueH),
                              juce::Justification::centredTop, 1, 0.75f);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (! enabledLook ? juce::MouseCursor::NormalCursor
                        : hasLed && labelArea().contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor
                                                                         : juce::MouseCursor::UpDownResizeCursor);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick (e); return; }
        if (hasLed && labelArea().contains (e.getPosition())) { if (onLedClick) onLedClick(); ledClick = true; return; }
        ledClick = false;
        if (! enabledLook) return;
        dragStart = value;
        lastPos = e.position;
        accum = value;
        begin();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging) return;
        const float fine = e.mods.isShiftDown() ? 0.1f : 1.0f;
        const auto d = e.position - lastPos;
        lastPos = e.position;
        accum = juce::jlimit (0.0f, 1.0f, accum + (d.x - d.y) / 200.0f * fine * sensitivity);
        apply (accum);
    }
    void mouseUp (const juce::MouseEvent&) override { end(); }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (! enabledLook || e.mods.isPopupMenu() || (hasLed && labelArea().contains (e.getPosition()))) return;
        if (onReset) { end(); onReset(); repaint(); return; }
        userSet (resetValue);
    }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! enabledLook) return;
        const float dir = std::abs (w.deltaY) >= std::abs (w.deltaX) ? w.deltaY : -w.deltaX;
        if (std::abs (dir) < 1e-6f) return;
        float step = steps > 1 ? 1.0f / (float) (steps - 1) : (e.mods.isShiftDown() ? 0.005f : 0.03f);
        step *= (w.isReversed ? -1.0f : 1.0f) * (dir > 0 ? 1.0f : -1.0f);
        userSet (juce::jlimit (0.0f, 1.0f, value + step));
    }

private:
    void begin() { if (! dragging) { dragging = true; if (onGestureStart) onGestureStart(); } }
    void end() { if (dragging) { dragging = false; if (onGestureEnd) onGestureEnd(); repaint(); } }
    void apply (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (steps > 1) v = std::round (v * (float) (steps - 1)) / (float) (steps - 1);
        if (std::abs (v - value) < 1.0e-6f) return;
        value = v;
        repaint();
        if (onChange) onChange (v);
    }

    float value = 0.5f, dragStart = 0.5f, accum = 0.5f;
    juce::Point<float> lastPos;
    bool dragging = false, enabledLook = true, modulated = false, ledClick = false;
};

// a knob on a host parameter (gestures, host automation follows the knob and the knob follows the host)
class KnobAttachment
{
public:
    KnobAttachment (juce::RangedAudioParameter& p, Knob& k)
        : param (p), knob (k), att (p, [this] (float v) { knob.setValue (param.convertTo0to1 (v)); }, nullptr)
    {
        knob.onGestureStart = [this] { att.beginGesture(); };
        knob.onChange = [this] (float v) { att.setValueAsPartOfGesture (param.convertFrom0to1 (v)); };
        knob.onGestureEnd = [this] { att.endGesture(); };
        att.sendInitialUpdate();
    }
    ~KnobAttachment() { knob.onGestureStart = nullptr; knob.onChange = nullptr; knob.onGestureEnd = nullptr; }

private:
    juce::RangedAudioParameter& param;
    Knob& knob;
    juce::ParameterAttachment att;
};
} // namespace vui
