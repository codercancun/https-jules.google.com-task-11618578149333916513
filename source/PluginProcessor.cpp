#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr float kGainRangeDb = 15.0f;

inline float loadParam (std::atomic<float>* param, float fallback = 0.0f)
{
    return param != nullptr ? param->load() : fallback;
}

inline void processChannel (ThreeBandEQ& eq,
                            juce::AudioBuffer<float>& buffer,
                            int channelIndex)
{
    auto block = juce::dsp::AudioBlock<float> (buffer)
                     .getSubsetChannelBlock (static_cast<size_t> (channelIndex), 1);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    eq.process (ctx);
}
} // namespace

NESEQAudioProcessor::NESEQAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", makeParameterLayout())
{
    lowGainParam  = apvts.getRawParameterValue (ParamIDs::lowGain);
    midGainParam  = apvts.getRawParameterValue (ParamIDs::midGain);
    highGainParam = apvts.getRawParameterValue (ParamIDs::highGain);
    bypassParam   = apvts.getRawParameterValue (ParamIDs::bypass);
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
    params.reserve (4);

    params.push_back (std::make_unique<FloatParam> (
        juce::ParameterID { ParamIDs::lowGain, 1 },
        "Low",
        gainRange,
        0.0f,
        gainAttributes));

    params.push_back (std::make_unique<FloatParam> (
        juce::ParameterID { ParamIDs::midGain, 1 },
        "Mid",
        gainRange,
        0.0f,
        gainAttributes));

    params.push_back (std::make_unique<FloatParam> (
        juce::ParameterID { ParamIDs::highGain, 1 },
        "High",
        gainRange,
        0.0f,
        gainAttributes));

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
    spec.numChannels      = 1; // each ThreeBandEQ instance handles one channel

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

    // Zero out any output channels beyond the input bus so we don't leak
    // garbage from earlier processing.
    for (auto ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const bool bypassed = bypassParam != nullptr && bypassParam->load() > 0.5f;
    if (bypassed)
        return;

    const float lowDb  = loadParam (lowGainParam);
    const float midDb  = loadParam (midGainParam);
    const float highDb = loadParam (highGainParam);

    eqLeft .update (lowDb, midDb, highDb);
    eqRight.update (lowDb, midDb, highDb);

    if (totalNumInputChannels > 0)
        processChannel (eqLeft, buffer, 0);

    if (totalNumInputChannels > 1)
        processChannel (eqRight, buffer, 1);
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
