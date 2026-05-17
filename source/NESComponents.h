#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "NESLookAndFeel.h"

namespace neseq
{
/**
    A vertical "power meter" style slider used for each EQ band. The slider's
    value is shown as a stack of chunky LED segments — lit green below the
    centre for cuts, red above for boosts — on a dark recessed well with a
    pixelated border, evoking an 8-bit UI element.
*/
class PowerMeterSlider final : public juce::Component
{
public:
    explicit PowerMeterSlider (juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& paramID,
                               const juce::String& bandLabel);

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() noexcept { return slider; }

private:
    juce::Slider slider;
    juce::Label  valueLabel;
    juce::Label  bandLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;

    static constexpr int kNumSegments = 15;

    void paintSegments (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerMeterSlider)
};

/**
    A round, red, NES controller-style "A" button used as the bypass toggle.
    Pressed state visually depresses the button and flips to a dimmer red,
    mirroring the way a game pad button feels when held down.
*/
class NESAButton final : public juce::Button
{
public:
    explicit NESAButton (juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID);

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
    juce::Font buttonFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESAButton)
};
} // namespace neseq
