#include "NESComponents.h"

namespace neseq
{
// =============================================================================
// PowerMeterSlider
// =============================================================================

PowerMeterSlider::PowerMeterSlider (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramID,
                                    const juce::String& label)
    : attachment (apvts, paramID, slider)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setDoubleClickReturnValue (true, 0.0);
    // Hide the built-in track/thumb — we render the whole thing ourselves.
    slider.setColour (juce::Slider::trackColourId,         juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::backgroundColourId,    juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::thumbColourId,         juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);

    addAndMakeVisible (slider);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setInterceptsMouseClicks (false, false);
    valueLabel.setColour (juce::Label::textColourId, NesPalette::yellow);
    addAndMakeVisible (valueLabel);

    bandLabel.setText (label, juce::dontSendNotification);
    bandLabel.setJustificationType (juce::Justification::centred);
    bandLabel.setInterceptsMouseClicks (false, false);
    bandLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (bandLabel);

    slider.onValueChange = [this]
    {
        valueLabel.setText (juce::String (slider.getValue(), 1) + " dB",
                            juce::dontSendNotification);
        repaint();
    };

    // Initial label sync.
    valueLabel.setText (juce::String (slider.getValue(), 1) + " dB",
                        juce::dontSendNotification);
}

void PowerMeterSlider::resized()
{
    auto area = getLocalBounds();
    bandLabel.setBounds (area.removeFromTop (20));
    valueLabel.setBounds (area.removeFromBottom (20));

    // Slider occupies the middle region; painting uses the same rect.
    slider.setBounds (area);
}

void PowerMeterSlider::paint (juce::Graphics& g)
{
    paintSegments (g, slider.getBounds());
}

void PowerMeterSlider::paintSegments (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw recessed well.
    g.setColour (NesPalette::darkGrey);
    g.fillRect (area);

    // Chunky pixel border (two pixels of light + two pixels of shadow).
    g.setColour (NesPalette::black);
    g.drawRect (area, 2);
    g.setColour (NesPalette::grey);
    g.drawRect (area.reduced (2), 2);

    const auto segmentsArea = area.reduced (6);
    if (segmentsArea.getHeight() <= 0)
        return;

    const auto segH = juce::jmax (2, segmentsArea.getHeight() / kNumSegments);
    const auto segW = segmentsArea.getWidth();
    const auto segX = segmentsArea.getX();
    const auto startY = segmentsArea.getY();

    const auto range     = slider.getRange();
    const auto normalised = range.getLength() > 0.0
        ? (slider.getValue() - range.getStart()) / range.getLength()
        : 0.5;

    // Value is -15..+15 dB; segment 0 is the bottom (cut), last segment top (boost),
    // middle segment represents 0 dB.
    const int midSegment = kNumSegments / 2;
    const int litSegment = juce::jlimit (0, kNumSegments - 1,
                                          static_cast<int> (std::round (normalised * (kNumSegments - 1))));

    for (int i = 0; i < kNumSegments; ++i)
    {
        // i=0 at bottom
        const auto y = startY + (kNumSegments - 1 - i) * segH;
        juce::Rectangle<int> seg (segX + 2, y + 1, segW - 4, segH - 2);

        juce::Colour segColour;
        bool lit = false;
        if (i == midSegment)
        {
            // centre line is always a dim amber "0 dB" tick
            segColour = NesPalette::yellow.withAlpha (0.9f);
            lit = true;
        }
        else if (i > midSegment && i <= litSegment)
        {
            // boost — red
            segColour = NesPalette::red;
            lit = true;
        }
        else if (i < midSegment && i >= litSegment)
        {
            // cut — green
            segColour = NesPalette::green;
            lit = true;
        }
        else
        {
            segColour = NesPalette::black;
        }

        g.setColour (segColour);
        g.fillRect (seg);

        if (lit)
        {
            // A thin highlight pixel on the top edge to sell the LED look.
            g.setColour (segColour.brighter (0.8f));
            g.fillRect (seg.getX(), seg.getY(), seg.getWidth(), 1);
        }
    }
}

// =============================================================================
// NESAButton
// =============================================================================

NESAButton::NESAButton (juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& paramID)
    : juce::Button ("A"),
      attachment (apvts, paramID, *this)
{
    setClickingTogglesState (true);
    setTooltip ("Bypass");

    buttonFont = juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold);
}

void NESAButton::resized()
{
    juce::Button::resized();
    const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    if (bounds.getHeight() > 0)
    {
        buttonFont = juce::Font (juce::Font::getDefaultMonospacedFontName(),
                                 bounds.getHeight() * 0.55f,
                                 juce::Font::bold);
    }
}

void NESAButton::paintButton (juce::Graphics& g,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const bool pressed = getToggleState() || shouldDrawButtonAsDown;

    const auto outerColour = pressed ? NesPalette::redShadow
                                     : NesPalette::red;
    const auto innerColour = pressed ? NesPalette::red
                                     : NesPalette::orange;

    // Outer pixel-circle body
    g.setColour (NesPalette::black);
    g.fillEllipse (bounds.expanded (2.0f));

    g.setColour (outerColour);
    g.fillEllipse (bounds);

    // Inner disc
    g.setColour (innerColour);
    g.fillEllipse (bounds.reduced (bounds.getWidth() * 0.18f));

    // Letter "A" in white, pixelated
    g.setColour (NesPalette::white);
    g.setFont (buttonFont);
    g.drawFittedText ("A",
                      getLocalBounds(),
                      juce::Justification::centred,
                      1);

    // Subtle highlight to hint at a 3D form when not pressed.
    if (! pressed)
    {
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillEllipse (bounds.getX() + bounds.getWidth() * 0.2f,
                       bounds.getY() + bounds.getHeight() * 0.12f,
                       bounds.getWidth() * 0.35f,
                       bounds.getHeight() * 0.18f);
    }

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour (NesPalette::yellow.withAlpha (0.4f));
        g.drawEllipse (bounds, 2.0f);
    }
}
} // namespace neseq
