#include "ui/Theme.h"

#include "DGData.h"

namespace digga::theme
{

Typefaces::Typefaces()
    : display  (juce::Typeface::createSystemTypefaceFor (DGData::ArchivoBlackRegular_ttf,
                                                         DGData::ArchivoBlackRegular_ttfSize)),
      condensed (juce::Typeface::createSystemTypefaceFor (DGData::BarlowCondensedSemiBold_ttf,
                                                          DGData::BarlowCondensedSemiBold_ttfSize)),
      condensedBold (juce::Typeface::createSystemTypefaceFor (DGData::BarlowCondensedBold_ttf,
                                                              DGData::BarlowCondensedBold_ttfSize)),
      mono     (juce::Typeface::createSystemTypefaceFor (DGData::CourierPrimeRegular_ttf,
                                                         DGData::CourierPrimeRegular_ttfSize)),
      monoBold (juce::Typeface::createSystemTypefaceFor (DGData::CourierPrimeBold_ttf,
                                                         DGData::CourierPrimeBold_ttfSize))
{
}

juce::Font display (float height, float horizontalScale)
{
    const juce::SharedResourcePointer<Typefaces> faces;
    return juce::Font (juce::FontOptions (faces->display).withHeight (height)
                                                         .withHorizontalScale (horizontalScale));
}

juce::Font condensed (float height, bool bold)
{
    const juce::SharedResourcePointer<Typefaces> faces;
    return juce::Font (juce::FontOptions (bold ? faces->condensedBold : faces->condensed).withHeight (height));
}

juce::Font mono (float height, bool bold)
{
    const juce::SharedResourcePointer<Typefaces> faces;
    return juce::Font (juce::FontOptions (bold ? faces->monoBold : faces->mono).withHeight (height));
}

juce::Path textPath (const juce::String& text, const juce::Font& font,
                     juce::Rectangle<float> area, bool stretch, juce::Justification justification)
{
    juce::GlyphArrangement glyphs;
    glyphs.addLineOfText (font, text, 0.0f, 0.0f);

    juce::Path path;
    glyphs.createPath (path);

    if (! path.isEmpty())
        path.applyTransform (path.getTransformToScaleToFit (area, ! stretch, justification));

    return path;
}

void addGrunge (juce::Graphics& g, const juce::Path& clip, juce::Colour speckColour,
                int seed, float density)
{
    const auto bounds = clip.getBounds();
    if (bounds.isEmpty())
        return;

    const juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (clip);

    juce::Random random (seed);
    const int numSpecks = (int) (bounds.getWidth() * bounds.getHeight() * density * 0.01f);

    for (int i = 0; i < numSpecks; ++i)
    {
        const float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
        const float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
        const float r = 0.3f + std::pow (random.nextFloat(), 3.0f) * 2.2f;
        g.setColour (speckColour.withMultipliedAlpha (0.45f + 0.55f * random.nextFloat()));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f * (0.6f + 0.8f * random.nextFloat()));
    }

    // a few scratches
    for (int i = 0; i < numSpecks / 60; ++i)
    {
        const float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
        const float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
        const float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
        const float length = 3.0f + random.nextFloat() * 14.0f;
        g.setColour (speckColour.withMultipliedAlpha (0.35f + 0.4f * random.nextFloat()));
        g.drawLine (x, y, x + std::cos (angle) * length, y + std::sin (angle) * length,
                    0.4f + random.nextFloat() * 0.6f);
    }
}

juce::Image skinBackground()
{
    return juce::ImageCache::getFromMemory (DGData::background_png, DGData::background_pngSize);
}

juce::Image skinDropSample()
{
    return juce::ImageCache::getFromMemory (DGData::drop_sample_png, DGData::drop_sample_pngSize);
}

juce::Image skinKillStamp()
{
    return juce::ImageCache::getFromMemory (DGData::kill_stamp_png, DGData::kill_stamp_pngSize);
}

} // namespace digga::theme
