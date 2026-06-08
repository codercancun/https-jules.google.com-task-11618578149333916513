#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr float kGainRangeDb = 15.0f;
} // namespace

NESEQAudioProcessor::NESEQAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", makeParameterLayout())
{
    for (int i = 0; i < EightBandEQ::kNumBands; ++i)
        bandGainParams[static_cast<size_t> (i)] = apvts.getRawParameterValue (ParamIDs::bandGain[static_cast<size_t> (i)]);

    bypassParam = apvts.getRawParameterValue (ParamIDs::bypass);
}

juce::AudioProcessorValueTreeState::ParameterLayout
NESEQAudioProcessor::makeParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam  = juce::AudioParameterBool;

    const auto gainRange = juce::NormalisableRange<float> (
        -kGainRangeDb, kGainRangeDb, 0.01f, 1.0f);

    auto gainAttributes = juce::AudioParameterFloatAttributes()
                              .withLabel ("dB")
                              .withStringFromValueFunction (
                                  [] (float value, int) {
                                      return juce::String (value, 1) + " dB";
                                  });

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve (EightBandEQ::kNumBands + 1);

    for (int i = 0; i < EightBandEQ::kNumBands; ++i)
    {
        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID { ParamIDs::bandGain[static_cast<size_t> (i)], 1 },
            EightBandEQ::kLabels[static_cast<size_t> (i)],
            gainRange,
            0.0f,
            gainAttributes));
    }

    params.push_back (std::make_unique<BoolParam> (
        juce::ParameterID { ParamIDs::bypass, 1 },
        "Bypass",
        false));

    return { params.begin(), params.end() };
}

void NESEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1; // each EightBandEQ instance handles one channel

    eqLeft.prepare (spec);
    eqRight.prepare (spec);
}

void NESEQAudioProcessor::releaseResources()
{
    eqLeft.reset();
    eqRight.reset();
}

bool NESEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void NESEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const bool bypassed = bypassParam != nullptr && bypassParam->load() > 0.5f;
    if (bypassed)
        return;

    std::array<float, EightBandEQ::kNumBands> gains {};
    for (int i = 0; i < EightBandEQ::kNumBands; ++i)
    {
        auto* p = bandGainParams[static_cast<size_t> (i)];
        gains[static_cast<size_t> (i)] = p != nullptr ? p->load() : 0.0f;
    }

    eqLeft .update (gains);
    eqRight.update (gains);

    if (totalNumInputChannels > 0)
    {
        auto leftBlock = juce::dsp::AudioBlock<float> (buffer)
                             .getSubsetChannelBlock (0, 1);
        juce::dsp::ProcessContextReplacing<float> ctx (leftBlock);
        eqLeft.process (ctx);
    }

    if (totalNumInputChannels > 1)
    {
        auto rightBlock = juce::dsp::AudioBlock<float> (buffer)
                              .getSubsetChannelBlock (1, 1);
        juce::dsp::ProcessContextReplacing<float> ctx (rightBlock);
        eqRight.process (ctx);
    }
}

juce::AudioProcessorEditor* NESEQAudioProcessor::createEditor()
{
    return new NESEQAudioProcessorEditor (*this);
}

void NESEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void NESEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}
} // namespace neseq

// This is the required entry point for hosts to instantiate the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new neseq::NESEQAudioProcessor();
}
