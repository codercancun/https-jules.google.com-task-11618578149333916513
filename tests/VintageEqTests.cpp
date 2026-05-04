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
        const float expectedGain = gainParam->getValue();

        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);

        gainParam->setValueNotifyingHost(0.2f);
        expect(gainParam->getValue() != expectedGain);

        processor.setStateInformation(stateData.getData(), (int) stateData.getSize());

        expectWithinAbsoluteError(gainParam->getValue(), expectedGain, 1.0e-4f);

        beginTest("Invalid state does not crash and leaves processor usable");

        char randomData[] = "Not an XML string!";
        processor.setStateInformation(randomData, sizeof(randomData));

        // Processor must remain usable after a bogus state load.
        gainParam->setValueNotifyingHost(0.5f);
        expectWithinAbsoluteError(gainParam->getValue(), 0.5f, 1.0e-4f);
    }
};

class VintageEqProcessBlockTest : public juce::UnitTest
{
public:
    VintageEqProcessBlockTest() : juce::UnitTest("VintageEqProcessBlockTest") {}

    void runTest() override
    {
        beginTest("Processing with valid sample rate modifies buffer");
        {
            VintageEqAudioProcessor processor;
            processor.setPlayConfigDetails(2, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            // Drive band 0 to a non-flat response so the filter is guaranteed to alter the signal.
            // setValueNotifyingHost takes a NORMALISED value in [0, 1].
            processor.treeState.getParameter("GAIN_0")->setValueNotifyingHost(1.0f); // +12 dB
            processor.treeState.getParameter("FREQ_0")->setValueNotifyingHost(0.5f);
            processor.treeState.getParameter("Q_0")->setValueNotifyingHost(0.5f);

            juce::AudioBuffer<float> buffer(2, 512);

            // A constant signal would be a DC component that a peak filter passes unchanged.
            // Use an alternating impulse train to produce broadband content.
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, (n % 2 == 0) ? 1.0f : -1.0f);

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            bool changed = false;
            for (int ch = 0; ch < buffer.getNumChannels() && ! changed; ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    if (std::abs(buffer.getSample(ch, n) - original.getSample(ch, n)) > 0.01f)
                    {
                        changed = true;
                        break;
                    }
            expect(changed);
        }

        beginTest("Processing with sample rate <= 0 exits early");
        {
            VintageEqAudioProcessor processor;
            // prepareToPlay has not been called: sample rate must be <= 0 for this test to be meaningful.
            expect(processor.getSampleRate() <= 0.0);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, 1.0f);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            bool unchanged = true;
            for (int ch = 0; ch < buffer.getNumChannels() && unchanged; ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    if (buffer.getSample(ch, n) != 1.0f)
                    {
                        unchanged = false;
                        break;
                    }
            expect(unchanged);
        }

        beginTest("Peak filter amplifies a sine at its centre frequency");
        {
            constexpr double sr        = 44100.0;
            constexpr int    blockSize = 1024;
            constexpr float  freqHz    = 1000.0f;

            VintageEqAudioProcessor processor;
            processor.setPlayConfigDetails (1, 1, sr, blockSize);
            processor.prepareToPlay (sr, blockSize);

            // Park every band flat (gain 0 dB) except band 0.
            for (int b = 0; b < 8; ++b)
                processor.treeState.getParameter ("GAIN_" + juce::String (b))->setValueNotifyingHost (0.5f); // 0 dB midpoint of [-12, 12]

            // Set band 0 to +12 dB at 1 kHz, raw values via AudioParameterFloat.
            auto setRaw = [&] (const juce::String& id, float raw)
            {
                auto* p = dynamic_cast<juce::AudioParameterFloat*> (processor.treeState.getParameter (id));
                expect (p != nullptr);
                *p = raw;
            };
            setRaw ("FREQ_0", freqHz);
            setRaw ("GAIN_0", 12.0f);
            setRaw ("Q_0",    1.0f);

            juce::AudioBuffer<float> buffer (1, blockSize);
            auto* data = buffer.getWritePointer (0);
            for (int n = 0; n < blockSize; ++n)
                data[n] = std::sin (juce::MathConstants<float>::twoPi * freqHz * (float) n / (float) sr);

            const float inputRms = buffer.getRMSLevel (0, 0, blockSize);

            juce::MidiBuffer midi;
            // A couple of warm-up blocks to let the IIR settle, then measure.
            processor.processBlock (buffer, midi);
            for (int n = 0; n < blockSize; ++n)
                data[n] = std::sin (juce::MathConstants<float>::twoPi * freqHz * (float) n / (float) sr);
            processor.processBlock (buffer, midi);

            const float outputRms = buffer.getRMSLevel (0, 0, blockSize);
            const float gainDb    = juce::Decibels::gainToDecibels (outputRms / inputRms);

            // +12 dB target; allow generous slack for IIR transient + Q shape.
            expect (gainDb > 6.0f);
        }

        beginTest("Processing clears extra output channels");
        {
            VintageEqAudioProcessor processor;
            // setPlayConfigDetails bypasses the BusesLayout negotiation and lets us simulate
            // a host providing a buffer with more output channels than input channels.
            processor.setPlayConfigDetails(1, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, 1.0f);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            // The extra output channel (index 1) must be cleared by processBlock.
            bool cleared = true;
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                if (std::abs(buffer.getSample(1, n)) > 0.01f)
                {
                    cleared = false;
                    break;
                }
            expect(cleared);
        }
    }
};

static VintageEqStateTest vintageEqStateTest;
static VintageEqProcessBlockTest vintageEqProcessBlockTest;

int main (int, char*[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult (i)->failures > 0)
            return 1;
    }
    return 0;
}
