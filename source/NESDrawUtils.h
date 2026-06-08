#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "NESLookAndFeel.h"

namespace neseq
{
namespace DrawUtils
{
/** Draw a chunky two-layer pixel border: 2px of @p outerColour then 2px of
    @p innerColour inset by 2 pixels. Used throughout the NES-EQ UI. */
inline void drawPixelBorder (juce::Graphics& g,
                             juce::Rectangle<int> area,
                             juce::Colour outerColour = NesPalette::black,
                             juce::Colour innerColour = NesPalette::grey)
{
    g.setColour (outerColour);
    g.drawRect (area, 2);
    g.setColour (innerColour);
    g.drawRect (area.reduced (2), 2);
}

/** Create a monospaced bold font at the given size — the standard NES-EQ
    "pixel text" look. */
inline juce::Font makeNESFont (float heightPixels)
{
    return juce::Font (juce::Font::getDefaultMonospacedFontName(),
                       heightPixels, juce::Font::bold);
}

/** Configure a label with common NES-EQ defaults: centred justification,
    no mouse interaction, specified text colour, and add it to a parent. */
inline void configureLabel (juce::Label& label,
                            juce::Component& parent,
                            juce::Colour textColour = NesPalette::white)
{
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    label.setColour (juce::Label::textColourId, textColour);
    parent.addAndMakeVisible (label);
}
} // namespace DrawUtils
} // namespace neseq
