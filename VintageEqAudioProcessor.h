#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class VintageEqAudioProcessor  : public juce::AudioProcessor
{
public:
    VintageEqAudioProcessor();
    ~VintageEqAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram ([[maybe_unused]] int index) override;
    const juce::String getProgramName ([[maybe_unused]] int index) override;
    void changeProgramName ([[maybe_unused]] int index, [[maybe_unused]] const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState treeState;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Filtros para las 8 bandas (Estilo API 550b)
    juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>,
                              juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>,
                              juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>,
                              juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>> eqChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageEqAudioProcessor)
};