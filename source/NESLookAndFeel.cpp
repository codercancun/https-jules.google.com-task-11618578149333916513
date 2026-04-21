#include "NESLookAndFeel.h"

namespace neseq
{
NESLookAndFeel::NESLookAndFeel()
{
    // Keep a monospaced, lightly kerned font family so text retains a pixel
    // feel even without shipping a custom bitmap font.
    pixelTypeface = juce::Font (juce::Font::getDefaultMonospacedFontName(),
                                 14.0f, juce::Font::bold).getTypefacePtr();

    // Overall palette tweaks for built-in components.
    setColour (juce::ResizableWindow::backgroundColourId, NesPalette::black);
    setColour (juce::Label::textColourId,                 NesPalette::white);
    setColour (juce::Label::backgroundColourId,           juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId,   NesPalette::darkGrey);
    setColour (juce::TooltipWindow::textColourId,         NesPalette::white);
    setColour (juce::TooltipWindow::outlineColourId,      NesPalette::lightGrey);
}

juce::Typeface::Ptr NESLookAndFeel::getTypefaceForFont (const juce::Font&)
{
    return pixelTypeface;
}

juce::Font NESLookAndFeel::getLabelFont (juce::Label& label)
{
    auto f = juce::LookAndFeel_V4::getLabelFont (label);
    f.setTypefaceName (juce::Font::getDefaultMonospacedFontName());
    f.setStyleFlags (juce::Font::bold);
    return f;
}

void NESLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // Plain text on transparent background – no bevelled borders.
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        const auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        g.setColour (label.findColour (juce::Label::textColourId)
                         .withMultipliedAlpha (alpha));
        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(),
                          label.getLocalBounds(),
                          label.getJustificationType(),
                          juce::jmax (1, label.getHeight() / 12),
                          label.getMinimumHorizontalScale());
    }
}
} // namespace neseq
