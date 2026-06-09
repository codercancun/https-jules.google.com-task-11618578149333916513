#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr float kGainRangeDb   = 15.0f;
constexpr float kOutputRangeDb = 12.0f;
} // namespace

NESEQAudioProcessor::NESEQAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", makeParameterLayout())
{
    lowGainParam    = apvts.getRawParameterValue (ParamIDs::lowGain);
    midGainParam    = apvts.getRawParameterValue (ParamIDs::midGain);
    highGainParam   = apvts.getRawParameterValue (ParamIDs::highGain);
    outputGainParam = apvts.getRawParameterValue (ParamIDs::outputGain);
    bypassParam     = apvts.getRawParameterValue (ParamIDs::bypass);

    jassert (lowGainParam    != nullptr);
    jassert (midGainParam    != nullptr);
    jassert (highGainParam   != nullptr);
    jassert (outputGainParam != nullptr);
    jassert (bypassParam     != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout
NESEQAudioProcessor::makeParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam  = juce::AudioParameterBool;

    const auto gainRange = juce::NormalisableRange<float> (
        -kGainRangeDb, kGainRangeDb, 0.01f, 1.0f);

    const auto outputRange = juce::NormalisableRange<float> (
        -kOutputRangeDb, kOutputRangeDb, 0.01f, 1.0f);

    auto gainAttributes = juce::AudioParameterFloatAttributes()
                              .withLabel ("dB")
                              .withStringFromValueFunction (
                                  [] (float value, int) {
                                      return juce::String (value, 1) + " dB";
                                  });

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.reserve (5);

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

    params.push_back (std::make_unique<FloatParam> (
        juce::ParameterID { ParamIDs::outputGain, 1 },
        "Output",
        outputRange,
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
    jassert (sampleRate > 0.0);
    jassert (samplesPerBlock > 0);

    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1;

    eqLeft.prepare (spec);
    eqRight.prepare (spec);

    bypassCrossfader.prepare (sampleRate, getTotalNumOutputChannels(),
                              samplesPerBlock);

    smoothedOutputGain.reset (sampleRate, 0.02);
    smoothedOutputGain.setCurrentAndTargetValue (1.0f);
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
    bypassCrossfader.setBypassed (bypassed);

    if (bypassCrossfader.shouldCaptureDry())
        bypassCrossfader.captureDry (buffer);

    if (bypassCrossfader.shouldRunEffect())
    {
        const float lowDb  = lowGainParam  != nullptr ? lowGainParam->load()  : 0.0f;
        const float midDb  = midGainParam  != nullptr ? midGainParam->load()  : 0.0f;
        const float highDb = highGainParam != nullptr ? highGainParam->load() : 0.0f;

        eqLeft .update (lowDb, midDb, highDb);
        eqRight.update (lowDb, midDb, highDb);

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

        // Apply smoothed output gain
        const float outDb = outputGainParam != nullptr ? outputGainParam->load() : 0.0f;
        smoothedOutputGain.setTargetValue (juce::Decibels::decibelsToGain (outDb));

        if (smoothedOutputGain.isSmoothing())
        {
            const auto numSamples = buffer.getNumSamples();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                auto gain = smoothedOutputGain;
                for (int i = 0; i < numSamples; ++i)
                    data[i] *= gain.getNextValue();
            }
            smoothedOutputGain.skip (numSamples);
        }
        else
        {
            const auto gain = smoothedOutputGain.getCurrentValue();
            if (std::abs (gain - 1.0f) > 1.0e-6f)
                buffer.applyGain (gain);
        }
    }

    bypassCrossfader.mix (buffer);
}

void NESEQAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= static_cast<int> (kFactoryPresets.size()))
        return;

    currentPreset.store (index);
    const auto& preset = kFactoryPresets[static_cast<size_t> (index)];

    if (auto* p = apvts.getParameter (ParamIDs::lowGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.lowDb));
    if (auto* p = apvts.getParameter (ParamIDs::midGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.midDb));
    if (auto* p = apvts.getParameter (ParamIDs::highGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.highDb));
}

const juce::String NESEQAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < static_cast<int> (kFactoryPresets.size()))
        return kFactoryPresets[static_cast<size_t> (index)].name;
    return {};
}

juce::AudioProcessorEditor* NESEQAudioProcessor::createEditor()
{
    return new NESEQAudioProcessorEditor (*this);
}

void NESEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        state.setProperty ("currentPreset", currentPreset.load(), nullptr);

        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void NESEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset.store (static_cast<int> (tree.getProperty ("currentPreset", 0)));
            apvts.replaceState (tree);
        }
    }
}
} // namespace neseq

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new neseq::NESEQAudioProcessor();
}
