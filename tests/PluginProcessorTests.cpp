#include "../source/PluginProcessor.h"
#include <juce_core/juce_core.h>

namespace neseq
{

class PluginProcessorTests : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("PluginProcessor") {}

    void runTest() override
    {
        beginTest ("isBusesLayoutSupported");
        {
            NESEQAudioProcessor processor;

            // Mono in, Mono out
            juce::AudioProcessor::BusesLayout monoLayout;
            monoLayout.inputBuses.add (juce::AudioChannelSet::mono());
            monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
            expect (processor.isBusesLayoutSupported (monoLayout), "Mono in / Mono out should be supported");

            // Stereo in, Stereo out
            juce::AudioProcessor::BusesLayout stereoLayout;
            stereoLayout.inputBuses.add (juce::AudioChannelSet::stereo());
            stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (processor.isBusesLayoutSupported (stereoLayout), "Stereo in / Stereo out should be supported");

            // Mismatched: Mono in, Stereo out
            juce::AudioProcessor::BusesLayout monoInStereoOut;
            monoInStereoOut.inputBuses.add (juce::AudioChannelSet::mono());
            monoInStereoOut.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (!processor.isBusesLayoutSupported (monoInStereoOut), "Mismatched (Mono in / Stereo out) should not be supported");

            // Mismatched: Stereo in, Mono out
            juce::AudioProcessor::BusesLayout stereoInMonoOut;
            stereoInMonoOut.inputBuses.add (juce::AudioChannelSet::stereo());
            stereoInMonoOut.outputBuses.add (juce::AudioChannelSet::mono());
            expect (!processor.isBusesLayoutSupported (stereoInMonoOut), "Mismatched (Stereo in / Mono out) should not be supported");

            // Matching but unsupported: LCR in, LCR out
            juce::AudioProcessor::BusesLayout lcrLayout;
            lcrLayout.inputBuses.add (juce::AudioChannelSet::createLCR());
            lcrLayout.outputBuses.add (juce::AudioChannelSet::createLCR());
            expect (!processor.isBusesLayoutSupported (lcrLayout), "LCR in / LCR out should not be supported");
        }
    }
};

static PluginProcessorTests pluginProcessorTests;
} // namespace neseq
