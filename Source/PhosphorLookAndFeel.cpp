#include "PhosphorLookAndFeel.h"
#include "LuckyNotesBinary.h"

namespace lnui
{
namespace
{
juce::Typeface::Ptr loadTypeface (const char* originalFileName)
{
    for (int i = 0; i < LuckyNotesBinary::namedResourceListSize; ++i)
    {
        if (juce::String (LuckyNotesBinary::originalFilenames[i]) == originalFileName)
        {
            int size = 0;
            if (const char* data = LuckyNotesBinary::getNamedResource (LuckyNotesBinary::namedResourceList[i], size))
                return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        }
    }
    return nullptr;
}

juce::Font makeFont (const juce::Typeface::Ptr& tf, float height, const juce::String& fallback)
{
    if (tf != nullptr)
        return juce::Font (juce::FontOptions (tf).withHeight (height));
    return juce::Font (juce::FontOptions (fallback, height, juce::Font::plain));
}

bool isRed (const juce::Button& b) { return b.getProperties()["ln-style"].toString() == "red"; }
bool isOutline (const juce::Button& b) { return b.getProperties()["ln-style"].toString() == "outline"; }
} // namespace

juce::Font pixelFont (float height)
{
    static juce::Typeface::Ptr tf = loadTypeface ("PressStart2P-Regular.ttf");
    return makeFont (tf, height, juce::Font::getDefaultMonospacedFontName());
}

juce::Font lcdFont (float height)
{
    static juce::Typeface::Ptr tf = loadTypeface ("VT323-Regular.ttf");
    return makeFont (tf, height, juce::Font::getDefaultMonospacedFontName());
}

juce::Font monoFont (float height, bool bold)
{
    static juce::Typeface::Ptr regular = loadTypeface ("IBMPlexMono-Regular.ttf");
    static juce::Typeface::Ptr boldTf = loadTypeface ("IBMPlexMono-Bold.ttf");
    return makeFont (bold ? boldTf : regular, height, juce::Font::getDefaultMonospacedFontName());
}

void drawGlowText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification j,
                   juce::Colour colour, const juce::Font& font, float glow)
{
    g.setFont (font);
    if (glow > 0.0f)
    {
        const float d = juce::jmax (1.0f, font.getHeight() * 0.06f) * glow;
        g.setColour (colour.withAlpha (0.10f));
        for (auto o : { juce::Point<float> (-d, 0), { d, 0 }, { 0, -d }, { 0, d }, { -d, -d }, { d, d }, { -d, d }, { d, -d } })
            g.drawText (text, area.translated (o.x, o.y), j, false);
        g.setColour (colour.withAlpha (0.16f));
        for (auto o : { juce::Point<float> (-d * 0.5f, 0), { d * 0.5f, 0 }, { 0, -d * 0.5f }, { 0, d * 0.5f } })
            g.drawText (text, area.translated (o.x, o.y), j, false);
    }
    g.setColour (colour);
    g.drawText (text, area, j, false);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (col::panel);
    g.fillRect (r);
    g.setColour (col::border);
    g.drawRect (r, 1.0f);
    // corner ticks
    g.setColour (col::dim.withAlpha (0.6f));
    const float t = 6.0f;
    for (auto c : { r.getTopLeft(), r.getTopRight().translated (-t, 0), r.getBottomLeft().translated (0, -1), r.getBottomRight().translated (-t, -1) })
        g.fillRect (c.x, c.y, t, 1.0f);
    if (title.isNotEmpty())
    {
        g.setFont (pixelFont (8.0f));
        g.setColour (col::dim);
        g.drawText (title, r.reduced (12, 0).withTrimmedTop (8).withHeight (10), juce::Justification::centredLeft, false);
    }
}

void drawLcd (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (col::lcdBg);
    g.fillRect (r);
    g.setColour (col::amber.withAlpha (0.05f));
    g.fillRect (r.reduced (3));
    g.setColour (juce::Colour (0xff3a2706));
    g.drawRect (r, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 1, r.getRight() - 1);
}

void drawPixelClover (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, bool glow)
{
    // four heart-shaped leaves around a centre pixel + a short stem (15 x 16 cells)
    static const char* const rows[] = {
        "...XXX...XXX...",
        "...XXXX.XXXX...",
        ".XX.XXX.XXX.XX.",
        ".XXXXXX.XXXXXX.",
        ".XXXXXX.XXXXXX.",
        "..XXXX...XXXX..",
        ".......X.......",
        "..XXXX...XXXX..",
        ".XXXXXX.XXXXXX.",
        ".XXXXXX.XXXXXX.",
        ".XX.XXX.XXX.XX.",
        "...XXXX.XXXX...",
        "...XXX...XXX...",
        ".......X.......",
        "........X......",
        "........X......",
    };
    constexpr int W = 15, H = 16;
    const float cell = juce::jmin (r.getWidth() / (float) W, r.getHeight() / (float) H);
    const auto origin = r.getCentre() - juce::Point<float> (cell * W * 0.5f, cell * H * 0.5f);
    auto paintCells = [&] (float grow, juce::Colour cc)
    {
        g.setColour (cc);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (rows[y][x] == 'X')
                    g.fillRect (origin.x + (float) x * cell - grow, origin.y + (float) y * cell - grow, cell + 2 * grow, cell + 2 * grow);
    };
    if (glow)
    {
        paintCells (cell * 0.9f, colour.withAlpha (0.08f));
        paintCells (cell * 0.4f, colour.withAlpha (0.16f));
    }
    paintCells (0.0f, colour);
    // tiny dark grid between the cells for the LED-matrix feel
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    const float line = juce::jmax (0.5f, cell * 0.1f);
    for (int i = 1; i < W; ++i) g.fillRect (origin.x + (float) i * cell, origin.y, line, cell * H);
    for (int i = 1; i < H; ++i) g.fillRect (origin.x, origin.y + (float) i * cell, cell * W, line);
}

void drawScanlines (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    for (int y = r.getY(); y < r.getBottom(); y += 3)
        g.fillRect (r.getX(), y, r.getWidth(), 1);

    juce::ColourGradient vignette (juce::Colours::transparentBlack, r.toFloat().getCentre(),
                                   juce::Colours::black.withAlpha (0.38f), r.toFloat().getTopLeft(), true);
    g.setGradientFill (vignette);
    g.fillRect (r);
}

// ---------------------------------------------------------------------------------------------------------------
PhosphorLookAndFeel::PhosphorLookAndFeel()
{
    setColour (juce::TooltipWindow::backgroundColourId, col::panel);
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::border);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::green.withAlpha (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, col::green);
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
}

void PhosphorLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool onState = b.getToggleState();

    if (isRed (b))
    {
        auto base = col::red;
        if (down) base = base.darker (0.25f);
        else if (over) base = base.brighter (0.12f);
        g.setGradientFill (juce::ColourGradient (base.brighter (0.15f), r.getX(), r.getY(), base.darker (0.35f), r.getX(), r.getBottom(), false));
        g.fillRect (r);
        // pixel bevel
        g.setColour (juce::Colours::white.withAlpha (down ? 0.08f : 0.28f));
        g.fillRect (r.getX(), r.getY(), r.getWidth(), 2.0f);
        g.fillRect (r.getX(), r.getY(), 2.0f, r.getHeight());
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRect (r.getX(), r.getBottom() - 3.0f, r.getWidth(), 3.0f);
        g.fillRect (r.getRight() - 3.0f, r.getY(), 3.0f, r.getHeight());
        g.setColour (juce::Colour (0xff6d0b11));
        g.drawRect (r, 1.0f);
        return;
    }

    g.setColour (col::bg);
    g.fillRect (r);
    if (onState)
    {
        g.setColour (col::green.withAlpha (0.14f));
        g.fillRect (r);
        g.setColour (col::green.withAlpha (0.10f));
        g.drawRect (r.expanded (1.5f), 2.0f);  // glow halo
        g.setColour (col::green);
        g.drawRect (r, 1.0f);
    }
    else
    {
        if (over) { g.setColour (col::green.withAlpha (0.06f)); g.fillRect (r); }
        g.setColour (isOutline (b) ? col::green.withAlpha (over ? 0.9f : 0.6f) : (over ? col::dim : col::border.brighter (0.25f)));
        g.drawRect (r, 1.0f);
    }
    if (down) { g.setColour (col::green.withAlpha (0.12f)); g.fillRect (r); }
}

juce::Font PhosphorLookAndFeel::getTextButtonFont (juce::TextButton& b, int h)
{
    if (isRed (b)) return pixelFont (juce::jmin (16.0f, (float) h * 0.3f));
    return pixelFont (juce::jmin (9.0f, (float) h * 0.4f));
}

void PhosphorLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    juce::ignoreUnused (down);
    const auto font = getTextButtonFont (b, b.getHeight());
    auto area = b.getLocalBounds().toFloat().reduced (4, 2);
    if (isRed (b))
    {
        drawGlowText (g, b.getButtonText(), area.translated (0, down ? 1.0f : 0.0f), juce::Justification::centred,
                      juce::Colour (0xfffff2f2), font, 0.6f);
        return;
    }
    const bool lit = b.getToggleState() || isOutline (b);
    const auto c = lit ? col::green : (over ? col::text : col::dim);
    drawGlowText (g, b.getButtonText(), area, juce::Justification::centred, c, font, lit ? 1.0f : 0.0f);
}

void PhosphorLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    g.fillAll (col::panel);
    g.setColour (col::green.withAlpha (0.6f));
    g.drawRect (0, 0, width, height, 1);
    g.setColour (col::text);
    g.setFont (monoFont (13.0f));
    g.drawFittedText (text, juce::Rectangle<int> (8, 4, width - 16, height - 8), juce::Justification::centredLeft, 4);
}

juce::Rectangle<int> PhosphorLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    const int w = juce::jmin (380, 24 + (int) juce::GlyphArrangement::getStringWidth (monoFont (13.0f), tipText));
    const int lines = 1 + (w >= 380 ? (int) (juce::GlyphArrangement::getStringWidth (monoFont (13.0f), tipText) / 340.0f) : 0);
    const int h = 10 + 18 * lines;
    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6, w, h)
        .constrainedWithin (parentArea);
}

void PhosphorLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (col::panel);
    g.setColour (col::border);
    g.drawRect (0, 0, width, height, 1);
}
} // namespace lnui
