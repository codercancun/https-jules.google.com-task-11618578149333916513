#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "NESComponents.h"
#include "NESLookAndFeel.h"
#include "PluginProcessor.h"

#include <array>
#include <memory>

namespace neseq
{
class NESEQAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit NESEQAudioProcessorEditor (NESEQAudioProcessor& processor);
    ~NESEQAudioProcessorEditor() override;

    void paint  (juce::Graphics& g) override;
    void resized() override;

private:
    NESEQAudioProcessor& audioProcessor;
    NESLookAndFeel lookAndFeel;

    std::array<std::unique_ptr<PowerMeterSlider>, EightBandEQ::kNumBands> bandSliders;

    NESAButton bypassButton { audioProcessor.getAPVTS(), ParamIDs::bypass };

    juce::Label titleLabel;
    juce::Label bypassLabel;

    void drawBackdrop (juce::Graphics& g, juce::Rectangle<int> area);
    void drawTitleBar (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESEQAudioProcessorEditor)
};
} // namespace neseq
