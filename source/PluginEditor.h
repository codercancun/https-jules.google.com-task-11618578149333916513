#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "NESComponents.h"
#include "NESLookAndFeel.h"
#include "PluginProcessor.h"
#include "Presets.h"
#include "SpectrumAnalyzer.h"

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

    PowerMeterSlider lowSlider  { audioProcessor.getAPVTS(), ParamIDs::lowGain,  "LOW"  };
    PowerMeterSlider midSlider  { audioProcessor.getAPVTS(), ParamIDs::midGain,  "MID"  };
    PowerMeterSlider highSlider { audioProcessor.getAPVTS(), ParamIDs::highGain, "HIGH" };

    PowerMeterSlider outputSlider { audioProcessor.getAPVTS(), ParamIDs::outputGain, "OUT" };

    NESAButton bypassButton { audioProcessor.getAPVTS(), ParamIDs::bypass };

    juce::Label titleLabel;
    juce::Label bypassLabel;

    // Presets
    juce::ComboBox presetSelector;
    juce::Label    presetLabel;
    void applyPreset (int presetIndex);

    // Spectrum analyser
    SpectrumAnalyzer analyzer { audioProcessor };

    void drawBackdrop (juce::Graphics& g, juce::Rectangle<int> area);
    void drawTitleBar (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESEQAudioProcessorEditor)
};
} // namespace neseq
