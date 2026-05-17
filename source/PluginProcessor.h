#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "ThreeBandEQ.h"

namespace neseq
{
/**
    Parameter ID constants for NES-EQ. Exposed so the editor can build
    attachments without string typos.
*/
namespace ParamIDs
{
    inline constexpr auto lowGain  = "low_gain";
    inline constexpr auto midGain  = "mid_gain";
    inline constexpr auto highGain = "high_gain";
    inline constexpr auto bypass   = "bypass";
}

class NESEQAudioProcessor : public juce::AudioProcessor
{
public:
    NESEQAudioProcessor();
    ~NESEQAudioProcessor() override = default;

    // juce::AudioProcessor ------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override {
#ifndef NES_EQ_UNIT_TESTS
        return JucePlugin_Name;
#else
        return "NES-EQ";
#endif
    }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Accessors -----------------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    ThreeBandEQ eqLeft;
    ThreeBandEQ eqRight;

    // Cached raw parameter pointers for lock-free access on the audio thread.
    std::atomic<float>* lowGainParam  = nullptr;
    std::atomic<float>* midGainParam  = nullptr;
    std::atomic<float>* highGainParam = nullptr;
    std::atomic<float>* bypassParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESEQAudioProcessor)
};
} // namespace neseq
