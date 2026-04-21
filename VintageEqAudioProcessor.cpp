#include "VintageEqAudioProcessor.h"

VintageEqAudioProcessor::VintageEqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       treeState (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

VintageEqAudioProcessor::~VintageEqAudioProcessor()
{
}

const juce::String VintageEqAudioProcessor::getName() const
{
    return "VintageEq";
}

bool VintageEqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VintageEqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VintageEqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VintageEqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VintageEqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VintageEqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VintageEqAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VintageEqAudioProcessor::getProgramName (int index)
{
    return {};
}

void VintageEqAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void VintageEqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();

    eqChain.prepare(spec);

    // Invalidate the cache to ensure coefficients are recalculated with the new sample rate
    for (int i = 0; i < 8; ++i)
    {
        cachedParams[i].freq = -1.0f;
    }
}

void VintageEqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool VintageEqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void VintageEqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto sampleRate = getSampleRate();
    if (sampleRate <= 0.0) return;

    for (int i = 0; i < 8; ++i)
    {
        auto freq = treeState.getRawParameterValue("FREQ_" + juce::String(i))->load();
        auto gain = treeState.getRawParameterValue("GAIN_" + juce::String(i))->load();
        auto q    = treeState.getRawParameterValue("Q_" + juce::String(i))->load();

        if (freq != cachedParams[i].freq || gain != cachedParams[i].gain || q != cachedParams[i].q)
        {
            cachedParams[i].freq = freq;
            cachedParams[i].gain = gain;
            cachedParams[i].q = q;

            auto filterCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gain));

            switch (i)
            {
                case 0: eqChain.template get<0>().coefficients = filterCoefficients; break;
                case 1: eqChain.template get<1>().coefficients = filterCoefficients; break;
                case 2: eqChain.template get<2>().coefficients = filterCoefficients; break;
                case 3: eqChain.template get<3>().coefficients = filterCoefficients; break;
                case 4: eqChain.template get<4>().coefficients = filterCoefficients; break;
                case 5: eqChain.template get<5>().coefficients = filterCoefficients; break;
                case 6: eqChain.template get<6>().coefficients = filterCoefficients; break;
                case 7: eqChain.template get<7>().coefficients = filterCoefficients; break;
            }
        }
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    eqChain.process(context);
}

bool VintageEqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VintageEqAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void VintageEqAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = treeState.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VintageEqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (treeState.state.getType()))
            treeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout VintageEqAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Define 8 bands
    for (int i = 0; i < 8; ++i)
    {
        // Frequency range typically 20Hz - 20kHz
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "FREQ_" + juce::String(i),
            "Frequency " + juce::String(i+1),
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
            1000.0f));

        // Gain range -12dB to +12dB
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "GAIN_" + juce::String(i),
            "Gain " + juce::String(i+1),
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f, 1.0f),
            0.0f));

        // Q (Resonance) range 0.1 to 10
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "Q_" + juce::String(i),
            "Q " + juce::String(i+1),
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 1.0f),
            0.707f));
    }

    return layout;
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VintageEqAudioProcessor();
}
