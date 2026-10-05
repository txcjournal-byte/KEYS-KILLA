// v0.45 CREATURE + DREAMS (included by PluginEditor.cpp).
// CreatureWidget: a small pet that lives in the plugin. It eats the sounds you skip and the ones you love (feed), grows with
// your taste (it takes the colour of what you like), gets hungry when nobody feeds it, and gets BORED when the same loop
// repeats too long (loopRepeated) - then it offers a SURPRISE (onSurprise).  Breathes, blinks, chews.  Remembered across sessions.
// DreamsPanel: "3 dreams from while you were away" - the DreamEngine's variations as cards: HEAR / KEEP.
#include "Dreams.h"
#include "SoundPack.h"

class CreatureWidget : public Component, private Timer
{
public:
    enum Mood { hungry, happy, bored, content };
    std::function<void()> onSurprise;

    CreatureWidget() { load(); last = Time::getMillisecondCounterHiRes(); startTimerHz (30); setRepaintsOnMouseActivity (false); }
    ~CreatureWidget() override { stopTimer(); save(); }

    // it eats a sound: liked = you kept / loved it (it grows and takes its colour), otherwise a skipped sound (just food)
    void feed (const String& soundName, bool liked)
    {
        eating = 1.0f; lastFood = soundName;
        hunger = jmax (0.0f, hunger - (liked ? 0.25f : 0.35f));
        if (liked)
        {
            joy = jmin (1.0f, joy + 0.25f); growth = jmin (1.0f, growth + 0.04f * (1.0f - growth) + 0.01f); ++liked_;
            const float h = (float) (hashName (soundName) % 360u) / 360.0f;
            float d = h - hue; if (d > 0.5f) d -= 1.0f; if (d < -0.5f) d += 1.0f;
            hue = std::fmod (hue + d * 0.25f + 1.0f, 1.0f);     // your taste colours it, a little at a time
            boredom = jmax (0.0f, boredom - 0.2f);
        }
        else joy = jmin (1.0f, joy + 0.05f);
        ++fed;
        for (int i = 0; i < 7; ++i) crumbs.push_back ({ (float) i * 0.9f, 1.0f });
        save(); repaint();
    }
    // the same loop came round again
    void loopRepeated() { boredom = jmin (1.0f, boredom + 0.15f); if (boredom >= 1.0f) joy = jmax (0.0f, joy - 0.1f); save(); repaint(); }

    Mood mood() const { return boredom >= 1.0f ? bored : hunger > 0.7f ? hungry : joy > 0.55f || eating > 0 ? happy : content; }
    String moodName() const { static const char* n[] { "HUNGRY", "HAPPY", "BORED", "CALM" }; return n[(int) mood()]; }
    float size() const { return growth; }
    int soundsEaten() const { return fed; }
    void debugState (float h, float j, float b, float gr, float hu, int f, int l) { hunger = h; joy = j; boredom = b; growth = gr; hue = hu; fed = f; liked_ = l; repaint(); }

    void mouseDown (const MouseEvent& e) override
    {
        if (mood() == bored && (bubbleArea().contains (e.position) || bodyArea().contains (e.position)))
        {
            boredom = 0; joy = jmin (1.0f, joy + 0.2f); wiggle = 1.0f; save();
            if (onSurprise) onSurprise();
            repaint(); return;
        }
        wiggle = 1.0f; joy = jmin (1.0f, joy + 0.03f);   // petting it
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const auto body = bodyArea();
        const auto c = body.getCentre();
        const float br = 1.0f + 0.035f * std::sin (phase * 1.6f) + 0.05f * eating * std::abs (std::sin (phase * 9.0f));
        const float R = body.getWidth() * 0.5f;
        const Mood m = mood();
        const auto col = Colour::fromHSV (hue, 0.35f + 0.45f * joy, m == hungry ? 0.65f : 0.95f, 1.0f);
        // its shadow and its glow
        g.setColour (Colours::black.withAlpha (0.25f)); g.fillEllipse (c.x - R * 0.75f, c.y + R * 0.92f, R * 1.5f, R * 0.22f);
        g.setColour (col.withAlpha (0.1f + 0.12f * joy)); g.fillEllipse (body.expanded (R * 0.14f));
        // antennae: one more for every few sounds you loved (up to 5), tipped with your taste
        const int ant = jlimit (1, 5, 1 + liked_ / 4);
        for (int k = 0; k < ant; ++k)
        {
            const float a = -MathConstants<float>::halfPi + ((float) k - (float) (ant - 1) * 0.5f) * 0.38f + 0.06f * std::sin (phase * 2.0f + (float) k);
            const auto base = c + Point<float> (std::cos (a), std::sin (a)) * R * 0.85f;
            const auto tip = c + Point<float> (std::cos (a), std::sin (a)) * R * (1.45f + 0.1f * std::sin (phase * 3.0f + (float) k));
            Path p; p.startNewSubPath (base); p.quadraticTo (base + (tip - base) * 0.5f + Point<float> (8.0f * std::sin (phase + (float) k), 0), tip);
            g.setColour (col.darker (0.4f)); g.strokePath (p, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colour::fromHSV (std::fmod (hue + 0.12f * (float) k, 1.0f), 0.8f, 1.0f, 1.0f)); g.fillEllipse (Rectangle<float> (9, 9).withCentre (tip));
        }
        // the body: a soft, breathing, slightly wobbling blob
        Path p; const int n = 64;
        for (int i = 0; i <= n; ++i)
        {
            const float a = (float) i / (float) n * MathConstants<float>::twoPi;
            float k = 1.0f + 0.04f * std::sin (a * 3.0f + phase * 1.1f) + 0.03f * std::sin (a * 5.0f - phase * 1.7f) + wiggle * 0.08f * std::sin (a * 7.0f + phase * 12.0f);
            if (std::sin (a) > 0) k *= 1.0f - 0.08f * std::sin (a) * (m == bored ? 1.5f : 1.0f);   // it sags a little when bored
            const auto pt = c + Point<float> (std::cos (a) * R * br * k * 1.05f, std::sin (a) * R * k / br);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        g.setGradientFill (ColourGradient (col.brighter (0.5f), c.x - R * 0.5f, c.y - R * 0.7f, col.darker (0.6f), c.x + R * 0.6f, c.y + R, true));
        g.fillPath (p);
        g.setColour (Colours::white.withAlpha (0.35f)); g.strokePath (p, PathStrokeType (1.5f));
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillEllipse (c.x - R * 0.55f, c.y - R * 0.6f, R * 0.4f, R * 0.22f);   // a shine
        // eyes
        const float ex = R * 0.32f, ey = c.y - R * 0.12f, er = R * (m == hungry ? 0.17f : 0.14f);
        const bool closed = blink > 0;
        for (int s = -1; s <= 1; s += 2)
        {
            const auto e = Point<float> (c.x + (float) s * ex, ey);
            if (closed) { g.setColour (Colour (0xff1b1d2a)); g.drawLine (e.x - er, e.y, e.x + er, e.y, 2.5f); continue; }
            g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (er * 2, er * 2.2f).withCentre (e));
            const auto look = Point<float> (std::sin (phase * 0.4f) * er * 0.35f, (m == hungry ? -0.2f : 0.15f) * er);
            g.setColour (Colour (0xff1b1d2a)); g.fillEllipse (Rectangle<float> (er * 1.1f, er * 1.2f).withCentre (e + look));
            g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (er * 0.35f, er * 0.35f).withCentre (e + look + Point<float> (-er * 0.2f, -er * 0.25f)));
            if (m == bored) { g.setColour (col.darker (0.2f)); g.fillRect (Rectangle<float> (er * 2.3f, er * 1.2f).withCentre (e - Point<float> (0, er * 0.6f))); }   // heavy lids
        }
        // mouth
        const auto mc = Point<float> (c.x, c.y + R * 0.28f);
        g.setColour (Colour (0xff1b1d2a));
        if (eating > 0)
        {
            const float open = R * (0.07f + 0.15f * std::abs (std::sin (phase * 9.0f + 1.0f)));
            g.fillEllipse (Rectangle<float> (R * 0.34f, 4 + open).withCentre (mc));
        }
        else if (m == hungry) g.drawEllipse (Rectangle<float> (R * 0.16f, R * 0.18f).withCentre (mc), 2.5f);
        else if (m == bored) g.drawLine (mc.x - R * 0.16f, mc.y, mc.x + R * 0.16f, mc.y + 2, 2.5f);
        else { Path s; s.addCentredArc (mc.x, mc.y - R * 0.08f, R * 0.22f, R * 0.16f * (0.5f + joy), 0, MathConstants<float>::pi * 0.62f, MathConstants<float>::pi * 1.38f, true); g.strokePath (s, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded)); }
        // cheeks when happy
        if (m == happy) { g.setColour (Colour (0xffff6b8a).withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (R * 0.2f, R * 0.1f).withCentre ({ c.x - ex * 1.45f, mc.y - R * 0.08f })); g.fillEllipse (Rectangle<float> (R * 0.2f, R * 0.1f).withCentre ({ c.x + ex * 1.45f, mc.y - R * 0.08f })); }
        // crumbs of the sound it is eating
        for (auto& cr : crumbs)
        {
            const float a = cr.x * 1.7f, d = R * (0.5f + 0.6f * (1.0f - cr.life));
            g.setColour (col.brighter (0.6f).withAlpha (cr.life)); g.fillEllipse (Rectangle<float> (5, 5).withCentre (mc + Point<float> (std::cos (a) * d, std::sin (a) * d * 0.5f + 6)));
        }
        // the bubble: what it feels
        if (m == bored)
        {
            const auto b = bubbleArea();
            g.setColour (Colours::white.withAlpha (0.92f)); g.fillRoundedRectangle (b, 14);
            Path tail; tail.addTriangle (b.getX() + 20, b.getBottom() - 1, b.getX() + 40, b.getBottom() - 1, b.getX() + 14, b.getBottom() + 14); g.fillPath (tail);
            g.setColour (Colour (0xff1b1d2a)); g.setFont (kk::modern::font (jmax (11.0f, b.getHeight() * 0.3f), true, 0.08f));
            g.drawFittedText ("this loop again ...\nSURPRISE ME?", b.reduced (8, 4).toNearestInt(), Justification::centred, 2, 0.8f);
        }
        // its name tag: mood + how much it ate
        const auto lab = getLocalBounds().toFloat().removeFromBottom (jmax (30.0f, (float) getHeight() * 0.14f));
        g.setColour (t.text); g.setFont (kk::modern::font (jmax (12.0f, lab.getHeight() * 0.45f), true, 0.3f));
        g.drawText (moodName(), lab.withHeight (lab.getHeight() * 0.55f).toNearestInt(), Justification::centred);
        g.setColour (t.dim); g.setFont (kk::modern::font (jmax (10.0f, lab.getHeight() * 0.3f), true, 0.05f));
        const String sub = eating > 0 && lastFood.isNotEmpty() ? "mmm ... " + lastFood : m == hungry ? String ("feed me a sound") : m == bored ? String ("click me") : String ("ate ") + String (fed) + (fed == 1 ? " sound" : " sounds");
        g.drawFittedText (sub, lab.withTrimmedTop (lab.getHeight() * 0.55f).toNearestInt(), Justification::centred, 1, 0.8f);
    }
private:
    struct Crumb { float x, life; };
    static uint32 hashName (const String& s) { uint32 h = 2166136261u; for (auto p = s.toUTF8(); *p != 0; ++p) { h ^= (uint8) *p; h *= 16777619u; } return h; }
    Rectangle<float> bodyArea() const
    {
        const auto r = getLocalBounds().toFloat().withTrimmedBottom (jmax (30.0f, (float) getHeight() * 0.14f)).withTrimmedTop ((float) getHeight() * 0.22f);
        const float d = jmin (r.getWidth(), r.getHeight()) * (0.5f + 0.3f * growth);
        return Rectangle<float> (d, d).withCentre (r.getCentre().translated (0, r.getHeight() * 0.06f));
    }
    Rectangle<float> bubbleArea() const { const auto b = bodyArea(); const float w = jmin ((float) getWidth() * 0.62f, 230.0f), h = jmax (40.0f, w * 0.32f); return { jmin (b.getCentreX() + 4, (float) getWidth() - w - 4), jmax (2.0f, b.getY() - h - 18), w, h }; }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const double now = Time::getMillisecondCounterHiRes();
        const float dt = (float) jlimit (0.0, 0.5, (now - last) / 1000.0); last = now;
        phase += dt * 2.0f;
        hunger = jmin (1.0f, hunger + dt / 900.0f);           // 15 minutes without food = hungry
        joy = jmax (0.0f, joy - dt / 1200.0f);
        eating = jmax (0.0f, eating - dt * 0.8f);
        wiggle = jmax (0.0f, wiggle - dt * 2.0f);
        for (auto& cr : crumbs) cr.life -= dt * 1.2f;
        crumbs.erase (std::remove_if (crumbs.begin(), crumbs.end(), [] (const Crumb& c) { return c.life <= 0; }), crumbs.end());
        if (blink > 0) blink -= dt; else if (Random::getSystemRandom().nextFloat() < dt * 0.3f) blink = 0.14f;
        repaint();
    }
    void load()
    {
        if (auto s = openSettings())
        {
            StringArray v; v.addTokens (s->getValue ("creature"), ",", "");
            if (v.size() == 7) { hunger = v[0].getFloatValue(); joy = v[1].getFloatValue(); boredom = v[2].getFloatValue(); growth = v[3].getFloatValue(); fed = v[4].getIntValue(); liked_ = v[5].getIntValue(); hue = v[6].getFloatValue(); }
        }
    }
    void save() const
    {
        if (auto s = openSettings())
            s->setValue ("creature", String (hunger, 3) + "," + String (joy, 3) + "," + String (boredom, 3) + "," + String (growth, 3) + "," + String (fed) + "," + String (liked_) + "," + String (hue, 3));
    }
    float hunger = 0.3f, joy = 0.5f, boredom = 0, growth = 0.1f, hue = 0.72f;
    int fed = 0, liked_ = 0;
    float phase = 0, eating = 0, blink = 0, wiggle = 0;
    double last = 0;
    String lastFood;
    std::vector<Crumb> crumbs;
};

// "3 dreams from while you were away": the DreamEngine's dreams as cards - HEAR (on the keys) / KEEP (MY SOUNDS)
class DreamsPanel : public Component
{
public:
    DreamsPanel (KeysKillaProcessor& p, KKLookAndFeel& l, DreamEngine& e) : proc (p), lnf (l), engine (e)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto& h = hear[(size_t) i]; auto& k = keep[(size_t) i];
            h.setButtonText ("HEAR"); h.framed = true; h.setTooltip ("Put this dream on your keys and hear it"); addChildComponent (h);
            k.setButtonText ("KEEP"); k.framed = true; k.setTooltip ("Keep this dream in MY SOUNDS"); addChildComponent (k);
            h.onClick = [this, i] { if (auto* d = dream (i)) { proc.alcUse (d->g, true); sel = i; note.clear(); repaint(); } };
            k.onClick = [this, i]
            {
                auto* d = dream (i); if (d == nullptr) return;
                engine.keep (i);
                const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
                auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (d->g, rate, 3.0), rate, rate, d->g.name);
                saveToFolderMenu (proc, { snd }, &keep[(size_t) i], [safe = SafePointer<DreamsPanel> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
            };
        }
        refresh();
    }
    // call when the engine dreamed / you came back
    void refresh()
    {
        for (int i = 0; i < 3; ++i) { hear[(size_t) i].setVisible (dream (i) != nullptr); keep[(size_t) i].setVisible (dream (i) != nullptr); }
        engine.markSeen();
        resized(); repaint();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const auto a = getLocalBounds().toFloat();
        kk::modern::well (g, a, 16.0f);
        // a night sky in the corner: the plugin was dreaming
        Random rr (5);
        for (int i = 0; i < 26; ++i) { g.setColour (Colours::white.withAlpha (0.1f + 0.3f * rr.nextFloat())); g.fillEllipse (a.getX() + 20 + rr.nextFloat() * (a.getWidth() - 40), a.getY() + 10 + rr.nextFloat() * 50, 2.2f, 2.2f); }
        {
            Graphics::ScopedSaveState ss (g);   // a crescent moon: the plugin was asleep
            const auto mc = Point<float> (a.getRight() - 42, a.getY() + 34);
            Path cut; cut.setUsingNonZeroWinding (false); cut.addRectangle (a); cut.addEllipse (Rectangle<float> (26, 26).withCentre (mc + Point<float> (7, -5)));
            g.reduceClipRegion (cut);
            g.setColour (Colour (0xffffe6a8).withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (40, 40).withCentre (mc));
            g.setColour (Colour (0xffffe6a8)); g.fillEllipse (Rectangle<float> (28, 28).withCentre (mc));
        }
        const int n = (int) engine.dreams().size();
        g.setColour (t.text); g.setFont (kk::modern::font (20.0f, true, 0.04f));
        g.drawText (n > 0 ? String (n) + (n == 1 ? " dream" : " dreams") + " from while you were away" : String ("no dreams yet"), a.reduced (20, 14).withHeight (28).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.05f));
        g.drawText (n > 0 ? String ("the plugin dreamed variations of the sounds you used") : String ("leave it alone for a while - it dreams variations of the sounds you used"),
                    a.reduced (20, 14).withTrimmedTop (30).withHeight (18).toNearestInt(), Justification::centredLeft);
        for (int i = 0; i < n && i < 3; ++i)
        {
            const auto& d = engine.dreams()[(size_t) i];
            const auto r = cardRect (i);
            const auto col = Colour::fromHSV ((float) (d.seed % 360u) / 360.0f, 0.45f, 0.9f, 1.0f);
            g.setGradientFill (ColourGradient (col.withAlpha (i == sel ? 0.32f : 0.18f), r.getX(), r.getY(), Colour (0xff0a0b18).withAlpha (0.6f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 14);
            g.setColour (i == sel ? Colours::white : Colours::white.withAlpha (0.2f)); g.drawRoundedRectangle (r.reduced (0.5f), 14, i == sel ? 2.0f : 1.0f);
            // a cloud with the dream inside
            const auto cc = Point<float> (r.getX() + 46, r.getCentreY() - 14);
            g.setColour (Colours::white.withAlpha (0.85f));
            for (auto [dx, dy, rad] : { std::tuple<float, float, float> { -14, 4, 13 }, { 0, -6, 17 }, { 16, 2, 14 }, { 4, 8, 12 } }) g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (cc + Point<float> (dx, dy)));
            g.setColour (col.darker (0.2f)); g.fillEllipse (Rectangle<float> (12, 12).withCentre (cc));
            g.setColour (Colours::white); g.setFont (kk::modern::font (16.0f, true, 0.03f));
            g.drawFittedText (d.g.name, r.withTrimmedLeft (92).withTrimmedRight (12).withTrimmedTop (12).withHeight (22).toNearestInt(), Justification::centredLeft, 1, 0.7f);
            g.setColour (Colours::white.withAlpha (0.65f)); g.setFont (kk::modern::font (11.5f, true, 0.05f));
            g.drawFittedText (d.how + (d.kept ? "   ·   kept" : ""), r.withTrimmedLeft (92).withTrimmedRight (12).withTrimmedTop (36).withHeight (18).toNearestInt(), Justification::centredLeft, 1, 0.8f);
            g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (11.0f, true, 0.05f));
            g.drawFittedText ("dreamed from  " + d.from, r.withTrimmedLeft (92).withTrimmedRight (12).withTrimmedTop (56).withHeight (16).toNearestInt(), Justification::centredLeft, 1, 0.8f);
            // the dream itself: a slow wave drifting across the card
            Path wv; const float x0 = r.getX() + 300, x1 = r.getRight() - 20, wy = r.getCentreY() + 14;
            for (int k = 0; k <= 60; ++k) { const float u = (float) k / 60.0f, x = x0 + u * (x1 - x0), y = wy + std::sin (u * 9.0f + (float) d.seed * 0.001f) * 14.0f * std::sin (u * MathConstants<float>::pi); if (k == 0) wv.startNewSubPath (x, y); else wv.lineTo (x, y); }
            if (x1 > x0 + 40) { g.setColour (col.withAlpha (0.55f)); g.strokePath (wv, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded)); }
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.04f));
        g.drawText (note.isNotEmpty() ? String() : String ("dreams come when FL is quiet.   HEAR = on your keys,  KEEP = into MY SOUNDS"), a.reduced (20, 8).withTrimmedTop (a.getHeight() - 30).toNearestInt(), Justification::centredLeft);
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.0f, true, 0.03f)); g.drawText (note, a.reduced (20, 8).withTrimmedTop (a.getHeight() - 30).toNearestInt(), Justification::centredLeft); }
    }
    void resized() override
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto r = cardRect (i).toNearestInt();
            const int bw = jmin (100, (r.getWidth() - 92 - 22) / 2);
            hear[(size_t) i].setBounds (r.getX() + 92, r.getBottom() - 40, bw, 32);
            keep[(size_t) i].setBounds (r.getX() + 92 + bw + 8, r.getBottom() - 40, bw, 32);
        }
    }
private:
    const DreamEngine::Dream* dream (int i) const { return isPositiveAndBelow (i, (int) engine.dreams().size()) ? &engine.dreams()[(size_t) i] : nullptr; }
    Rectangle<float> cardRect (int i) const
    {
        const auto a = getLocalBounds().toFloat().reduced (16).withTrimmedTop (60).withTrimmedBottom (24);
        const float h = jmin (172.0f, (a.getHeight() - 20) / 3);
        return { a.getX(), a.getY() + (float) i * (h + 10), a.getWidth(), h };
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; DreamEngine& engine;
    std::array<HotButton, 3> hear { HotButton (lnf), HotButton (lnf), HotButton (lnf) }, keep { HotButton (lnf), HotButton (lnf), HotButton (lnf) };
    String note;
    int sel = -1;
};
