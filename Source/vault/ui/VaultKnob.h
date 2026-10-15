#pragma once
#include "VaultTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>

// CHROME VAULT v3 knob: the small professional vector knob of the approved mockup - a dark body, a thin track arc, the ice
// value arc, a white pointer, the LABEL under it and the value text under that. A modulated knob shows a green ring
// (amount of the modulation) outside the track, its value arc turns green.
//   drag up / right = more (shift = fine), mouse wheel steps, double-click = the preset's value, right-click = menu
//   (the "Modulate with..." menu where a target exists). An optional LED in front of the label switches the effect on / off
//   (click the label).
// The knob never takes the keyboard focus. Value 0..1 (callers map it to the real range).
namespace vui
{
inline void drawKnobBody (juce::Graphics& g, juce::Rectangle<float> box, float value, float arcOrigin, float modAmount,
                          bool enabled, bool hot, bool modulated)
{
    using namespace juce;
    const float s = std::min (box.getWidth(), box.getHeight());
    const auto c = box.getCentre();
    const float r = s * 0.5f - 3.0f;                       // track radius
    const float a0 = MathConstants<float>::pi * -0.75f, a1 = MathConstants<float>::pi * 0.75f;
    const float ang = a0 + jlimit (0.0f, 1.0f, value) * (a1 - a0);
    const float sw = s >= 40.0f ? 3.0f : 2.6f;

    Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (col::track);
    g.strokePath (track, PathStrokeType (sw, PathStrokeType::curved, PathStrokeType::rounded));

    const float ao = a0 + jlimit (0.0f, 1.0f, arcOrigin) * (a1 - a0);
    if (arcOrigin > 0.001f && arcOrigin < 0.999f)
    {
        // bipolar: a small tick marks the centre (the value as designed)
        g.setColour (col::dim.withAlpha (0.8f));
        g.drawLine ({ c.getPointOnCircumference (r + sw * 0.5f + 1.0f, ao), c.getPointOnCircumference (r + sw * 0.5f + 3.5f, ao) }, 1.4f);
    }
    if (std::abs (ang - ao) > 0.02f)
    {
        Path v; v.addCentredArc (c.x, c.y, r, r, 0.0f, std::min (ao, ang), std::max (ao, ang), true);
        g.setColour (! enabled ? col::dim.withAlpha (0.6f) : modulated ? col::ok : col::ice);
        g.strokePath (v, PathStrokeType (sw, PathStrokeType::curved, PathStrokeType::rounded));
    }
    if (std::abs (modAmount) > 0.004f && enabled)
    {
        // the modulation range: a thin green arc just outside the track, from the value in the direction of the amount
        const float rr = r + sw * 0.5f + 2.2f;
        const float to = jlimit (a0, a1, ang + modAmount * (a1 - a0));
        Path m; m.addCentredArc (c.x, c.y, rr, rr, 0.0f, std::min (ang, to), std::max (ang, to), true);
        g.setColour (col::ok.withAlpha (0.9f));
        g.strokePath (m, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // body: a dark dome lit from the upper left
    const float br = r - sw * 0.5f - 2.5f;
    auto body = Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (c);
    ColourGradient grad (Colour (0xff3a4558), body.getX() + body.getWidth() * 0.4f, body.getY() + body.getHeight() * 0.3f,
                         Colour (0xff0e131b), body.getRight(), body.getBottom(), true);
    grad.addColour (0.6, Colour (0xff1a2230));
    g.setGradientFill (grad);
    g.fillEllipse (body);
    g.setColour (Colour (0xff05080d));
    g.drawEllipse (body, 1.0f);
    if (hot && enabled)
    {
        g.setColour (col::ice.withAlpha (0.28f));
        g.drawEllipse (body.expanded (0.5f), 1.0f);
    }
    // pointer
    const auto p0 = c.getPointOnCircumference (br * 0.18f, ang), p1 = c.getPointOnCircumference (br * 0.86f, ang);
    g.setColour (enabled ? col::text : col::dim);
    g.drawLine ({ p0, p1 }, s >= 40.0f ? 2.0f : 1.8f);
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
    bool hasLed = false, ledOn = false;
    int steps = 0;                           // > 1: snaps to steps (integer / choice parameters)
    float sensitivity = 1.0f;                // < 1 = slower drag
    std::function<juce::String (float)> valueText;   // the value line (default: percent)

    // behaviour
    float resetValue = 0.5f;                 // double-click target (the preset's value)
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
        drawKnobBody (g, knobArea(), value, arcOrigin, modulated ? modAmount : 0.0f, enabledLook, hot, modulated);
        // label (with the LED in front of it)
        auto lr = labelArea().toFloat();
        auto f = font (14.0f, true, 0.08f);
        juce::String txt = label;
        const float ledW = hasLed ? 11.0f : 0.0f;
        if (textWidth (f, txt) + ledW > lr.getWidth()) f = font (14.0f, true, 0.0f);
        if (textWidth (f, txt) + ledW > lr.getWidth() && shortLabel.isNotEmpty()) txt = shortLabel;
        while (textWidth (f, txt) + ledW > lr.getWidth() && txt.length() > 3) txt = txt.dropLastCharacters (1);   // never overlaps a neighbour
        const float tw = textWidth (f, txt) + ledW;
        float x = lr.getCentreX() - tw * 0.5f;
        if (hasLed)
        {
            const auto led = juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ x + 3.0f, lr.getCentreY() });
            if (ledOn)
            {
                g.setColour (col::ok.withAlpha (0.3f)); g.fillEllipse (led.expanded (2.0f));
                g.setColour (col::ok); g.fillEllipse (led);
            }
            else { g.setColour (col::ledOff); g.fillEllipse (led); }
            x += ledW;
        }
        g.setFont (f);
        g.setColour (enabledLook ? (hasLed && ! ledOn ? col::dim : col::muted) : col::dim);
        g.drawText (txt, juce::Rectangle<float> (x, lr.getY(), tw - ledW + 4.0f, lr.getHeight()), juce::Justification::centredLeft, false);
        // value
        g.setFont (font (14.0f));
        g.setColour (hot && enabledLook ? col::ice : col::dim);
        g.drawText (currentValueText(), juce::Rectangle<float> (-6.0f, lr.getBottom(), (float) getWidth() + 12.0f, (float) valueH),
                    juce::Justification::centredTop, false);
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
