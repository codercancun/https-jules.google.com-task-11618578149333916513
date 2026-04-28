#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "ThreeBandEQ.h"
#include <array>
#include <atomic>

namespace neseq
{
/**
    Parameter ID constants for NES-EQ. Exposed so the editor can build
    attachments without string typos.
*/
namespace ParamIDs
{
    inline constexpr auto lowGain    = "low_gain";
    inline constexpr auto midGain    = "mid_gain";
    inline constexpr auto highGain   = "high_gain";
    inline constexpr auto bypass     = "bypass";
    inline constexpr auto outputGain = "output_gain";
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

    const juce::String getName() const override { return JucePlugin_Name; }

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
    std::atomic<float>* lowGainParam    = nullptr;
    std::atomic<float>* midGainParam    = nullptr;
    std::atomic<float>* highGainParam   = nullptr;
    std::atomic<float>* bypassParam     = nullptr;
    std::atomic<float>* outputGainParam = nullptr;

    // ---- Spectrum analyser FIFO (single-producer / single-consumer) -----
    static constexpr int kFFTOrder = 10;                       // 1024-point FFT
    static constexpr int kFFTSize  = 1 << kFFTOrder;           // 1024

    std::array<float, kFFTSize * 2> fftData {};
    std::array<float, kFFTSize>     fifoBuffer {};
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    void pushSampleToFifo (float sample) noexcept;

public:
    // The editor reads the FFT result from this array.
    std::array<float, kFFTSize * 2>& getFFTData() noexcept       { return fftData; }
    bool& getFFTReady() noexcept                                  { return nextFFTBlockReady; }
    static constexpr int getFFTSize() noexcept                    { return kFFTSize; }
    static constexpr int getFFTOrder() noexcept                   { return kFFTOrder; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NESEQAudioProcessor)
};
} // namespace neseq
