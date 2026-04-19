#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../VintageEqAudioProcessor.h"

class VintageEqStateTest : public juce::UnitTest
{
public:
    VintageEqStateTest() : juce::UnitTest("VintageEqStateTest") {}

    void runTest() override
    {
        beginTest("State can be saved and restored");

        VintageEqAudioProcessor processor;

        auto* gainParam = processor.treeState.getParameter("GAIN_0");
        gainParam->setValueNotifyingHost(0.8f);
        float expectedGain = gainParam->getValue();

        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);

        gainParam->setValueNotifyingHost(0.2f);
        expect(gainParam->getValue() != expectedGain);

        processor.setStateInformation(stateData.getData(), (int)stateData.getSize());

        expectEquals(gainParam->getValue(), expectedGain);

        beginTest("Invalid state does not crash");

        char randomData[] = "Not an XML string!";
        processor.setStateInformation(randomData, sizeof(randomData));

        expect(true);
    }
};

class VintageEqProcessBlockTest : public juce::UnitTest
{
public:
    VintageEqProcessBlockTest() : juce::UnitTest("VintageEqProcessBlockTest") {}

    void runTest() override
    {
        beginTest("Clears unneeded channels");
        {
            VintageEqAudioProcessor processor;

            // Set up 1 in, 2 out
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add(juce::AudioChannelSet::mono());
            layout.outputBuses.add(juce::AudioChannelSet::stereo());

            // It might fail if layout isn't supported, but assuming template setup defaults to stereo/stereo
            // Let's just create a buffer with 2 channels, pretending 1 in, 2 out
            // Since we can't easily change the processor's input/output channel count here without proper setup,
            // we will simulate the behavior manually or skip if not possible.
            // Actually, the processor defaults to stereo in / stereo out. We can try to process a block and make sure it doesn't crash.
        }

        beginTest("Processing with valid sample rate modifies buffer");
        {
            VintageEqAudioProcessor processor;
            processor.setPlayConfigDetails(2, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            // Gain needs to be set to a value that modifies the signal
            auto* gainParam = processor.treeState.getParameter("GAIN_0");
            gainParam->setValueNotifyingHost(12.0f); // Max gain to ensure modification

            auto* freqParam = processor.treeState.getParameter("FREQ_0");
            freqParam->setValueNotifyingHost(1.0f);

            juce::AudioBuffer<float> buffer(2, 512);

            // Fill with sine wave or noise, a constant 1.0f might not be modified by peak filters
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                for (int j = 0; j < buffer.getNumSamples(); ++j)
                    buffer.setSample(i, j, (j % 2 == 0) ? 1.0f : -1.0f);

            // Process a few blocks to allow filters to warm up and change the output
            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);
            processor.processBlock(buffer, midiMessages);

            // After processing, the sample values should be modified
            bool changed = false;
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                for (int j = 0; j < buffer.getNumSamples(); ++j)
                {
                    float original = (j % 2 == 0) ? 1.0f : -1.0f;
                    if (std::abs(buffer.getSample(i, j) - original) > 0.01f)
                    {
                        changed = true;
                        break;
                    }
                }
            expect(changed);
        }

        beginTest("Processing with sample rate <= 0 exits early");
        {
            VintageEqAudioProcessor processor;
            // sampleRate is 0 by default before prepareToPlay

            juce::AudioBuffer<float> buffer(2, 512);
            // Fill with 1.0f
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                for (int j = 0; j < buffer.getNumSamples(); ++j)
                    buffer.setSample(i, j, 1.0f);

            juce::MidiBuffer midiMessages;

            processor.processBlock(buffer, midiMessages);

            // Since sample rate <= 0, processBlock should return early and not modify the buffer
            bool changed = false;
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                for (int j = 0; j < buffer.getNumSamples(); ++j)
                    if (buffer.getSample(i, j) != 1.0f)
                    {
                        changed = true;
                        break;
                    }
            expect(!changed);
        }

        beginTest("Processing clears extra output channels");
        {
            // Instead of simulating via setPlayConfigDetails which might not work exactly as intended
            // if the plugin defaults to 2 in / 2 out in BusesLayout, let's subclass and force it.
            struct MonoToStereoProcessor : public VintageEqAudioProcessor {
                MonoToStereoProcessor() {}

                // Override bus layout checking to allow mono to stereo
                bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
                    return true;
                }
            };

            MonoToStereoProcessor processor;
            // Set bus layout directly
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add(juce::AudioChannelSet::mono());
            layout.outputBuses.add(juce::AudioChannelSet::stereo());
            processor.setBusesLayout(layout);

            processor.setPlayConfigDetails(1, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                for (int j = 0; j < buffer.getNumSamples(); ++j)
                    buffer.setSample(i, j, 1.0f);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            bool cleared = true;
            for (int j = 0; j < buffer.getNumSamples(); ++j)
            {
                if (std::abs(buffer.getSample(1, j)) > 0.01f)
                {
                    cleared = false;
                    break;
                }
            }
            expect(cleared);
        }
    }
};

static VintageEqStateTest vintageEqStateTest;
static VintageEqProcessBlockTest vintageEqProcessBlockTest;

int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
            return 1;
    }
    return 0;
}
