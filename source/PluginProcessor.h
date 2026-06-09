#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "BypassCrossfader.h"
#include "Presets.h"
#include "ThreeBandEQ.h"

namespace neseq
{
namespace ParamIDs
{
    inline constexpr auto lowGain    = "low_gain";
    inline constexpr auto midGain    = "mid_gain";
    inline constexpr auto highGain   = "high_gain";
    inline constexpr auto outputGain = "output_gain";
    inline constexpr auto bypass     = "bypass";
}

class NESEQAudioProcessor : public juce::AudioProcessor
{
public:
    NESEQAudioProcessor();
    ~NESEQAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return static_cast<int> (kFactoryPresets.size()); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    ThreeBandEQ eqLeft;
    ThreeBandEQ eqRight;
    BypassCrossfader bypassCrossfader;

    std::atomic<float>* lowGainParam    = nullptr;
    std::atomic<float>* midGainParam    = nullptr;
    std::atomic<float>* highGainParam   = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* bypassParam     = nullptr;

    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESEQAudioProcessor)
};
} // namespace neseq
