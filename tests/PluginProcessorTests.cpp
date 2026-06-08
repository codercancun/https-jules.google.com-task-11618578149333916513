#include "../source/PluginProcessor.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <cstring>

namespace neseq
{
namespace
{
constexpr double kTestSampleRate = 48000.0;
constexpr int    kTestBlockSize  = 512;

void fillSine (juce::AudioBuffer<float>& buffer, float freqHz, double sampleRate)
{
    const double omega = juce::MathConstants<double>::twoPi * freqHz / sampleRate;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = static_cast<float> (std::sin (omega * i));
    }
}

float channelRms (const juce::AudioBuffer<float>& buffer, int channel)
{
    double sumSq = 0.0;
    const auto* data = buffer.getReadPointer (channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        sumSq += static_cast<double> (data[i]) * data[i];
    return static_cast<float> (std::sqrt (sumSq / buffer.getNumSamples()));
}
} // namespace

class PluginProcessorTests : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("PluginProcessor") {}

    void runTest() override
    {
        testParameterLayout();
        testBusLayouts();
        testBypassPassesThrough();
        testProcessBlockAppliesEQ();
        testStateRoundTrip();
        testProcessMonoLayout();
    }

private:
    void testParameterLayout()
    {
        beginTest ("parameter layout has all 8 band gains + bypass");
        {
            NESEQAudioProcessor proc;
            auto& apvts = proc.getAPVTS();

            for (int i = 0; i < EightBandEQ::kNumBands; ++i)
            {
                auto* p = apvts.getParameter (ParamIDs::bandGain[static_cast<size_t> (i)]);
                expect (p != nullptr, juce::String (ParamIDs::bandGain[static_cast<size_t> (i)])
                    + " parameter should exist");

                if (p != nullptr)
                {
                    expectWithinAbsoluteError (p->getDefaultValue(), 0.5f, 0.01f,
                        juce::String (ParamIDs::bandGain[static_cast<size_t> (i)])
                            + " default normalised should map to 0 dB");
                }
            }

            auto* byp = apvts.getParameter (ParamIDs::bypass);
            expect (byp != nullptr, "bypass parameter should exist");
            if (byp != nullptr)
                expectWithinAbsoluteError (byp->getDefaultValue(), 0.0f, 0.01f,
                    "bypass default should be off");

            // Check gain range on first band.
            auto* band1 = dynamic_cast<juce::AudioParameterFloat*> (
                apvts.getParameter (ParamIDs::bandGain[0]));
            expect (band1 != nullptr, "band1_gain should be AudioParameterFloat");
            if (band1 != nullptr)
            {
                const auto& range = band1->getNormalisableRange();
                expectWithinAbsoluteError (range.start, -15.0f, 0.01f,
                    "gain range start should be -15 dB");
                expectWithinAbsoluteError (range.end, 15.0f, 0.01f,
                    "gain range end should be +15 dB");
            }
        }
    }

    void testBusLayouts()
    {
        beginTest ("bus layout: stereo accepted, mono accepted, mismatch rejected");
        {
            NESEQAudioProcessor proc;

            juce::AudioProcessor::BusesLayout stereo;
            stereo.inputBuses .add (juce::AudioChannelSet::stereo());
            stereo.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (proc.isBusesLayoutSupported (stereo), "stereo should be supported");

            juce::AudioProcessor::BusesLayout mono;
            mono.inputBuses .add (juce::AudioChannelSet::mono());
            mono.outputBuses.add (juce::AudioChannelSet::mono());
            expect (proc.isBusesLayoutSupported (mono), "mono should be supported");

            juce::AudioProcessor::BusesLayout mismatch;
            mismatch.inputBuses .add (juce::AudioChannelSet::mono());
            mismatch.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (! proc.isBusesLayoutSupported (mismatch), "mismatch should be rejected");

            juce::AudioProcessor::BusesLayout surround;
            surround.inputBuses .add (juce::AudioChannelSet::create5point1());
            surround.outputBuses.add (juce::AudioChannelSet::create5point1());
            expect (! proc.isBusesLayoutSupported (surround), "surround should be rejected");
        }
    }

    void testBypassPassesThrough()
    {
        beginTest ("processBlock with bypass produces unmodified output");
        {
            NESEQAudioProcessor proc;
            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Set a non-zero EQ gain.
            auto* band1 = proc.getAPVTS().getParameter (ParamIDs::bandGain[0]);
            if (band1 != nullptr) band1->setValueNotifyingHost (0.8f);

            // Enable bypass.
            auto* byp = proc.getAPVTS().getParameter (ParamIDs::bypass);
            if (byp != nullptr) byp->setValueNotifyingHost (1.0f);

            juce::AudioBuffer<float> buffer (2, kTestBlockSize);
            fillSine (buffer, 200.0f, kTestSampleRate);
            juce::AudioBuffer<float> reference (buffer);

            juce::MidiBuffer midi;
            proc.processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
            {
                const bool match = std::memcmp (buffer.getReadPointer (ch),
                                                reference.getReadPointer (ch),
                                                sizeof (float) * static_cast<size_t> (kTestBlockSize)) == 0;
                expect (match, "channel " + juce::String (ch) + " should be unmodified when bypassed");
            }

            proc.releaseResources();
        }
    }

    void testProcessBlockAppliesEQ()
    {
        beginTest ("processBlock applies 8-band EQ when not bypassed");
        {
            NESEQAudioProcessor proc;
            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Boost the sub band (band 0) heavily.
            auto* band1 = proc.getAPVTS().getParameter (ParamIDs::bandGain[0]);
            if (band1 != nullptr) band1->setValueNotifyingHost (0.9f);

            auto* byp = proc.getAPVTS().getParameter (ParamIDs::bypass);
            if (byp != nullptr) byp->setValueNotifyingHost (0.0f);

            juce::AudioBuffer<float> buffer (2, kTestBlockSize);
            juce::MidiBuffer midi;
            float lastRmsL = 0.0f;

            for (int block = 0; block < 64; ++block)
            {
                fillSine (buffer, 30.0f, kTestSampleRate);
                proc.processBlock (buffer, midi);
                if (block >= 32)
                    lastRmsL = channelRms (buffer, 0);
            }

            const float inputRms = 1.0f / std::sqrt (2.0f);
            expectGreaterThan (lastRmsL, inputRms * 2.0f,
                "sub-frequency signal should be significantly boosted");

            // Both channels should be processed identically.
            fillSine (buffer, 30.0f, kTestSampleRate);
            proc.processBlock (buffer, midi);
            const float rmsL = channelRms (buffer, 0);
            const float rmsR = channelRms (buffer, 1);
            expectWithinAbsoluteError (rmsL, rmsR, 0.01f,
                "left and right channels should receive identical EQ");

            proc.releaseResources();
        }
    }

    void testStateRoundTrip()
    {
        beginTest ("state save / restore preserves parameter values");
        {
            juce::MemoryBlock stateBlock;

            {
                NESEQAudioProcessor procA;
                // Set some non-default values.
                auto* b1 = procA.getAPVTS().getParameter (ParamIDs::bandGain[0]);
                auto* b4 = procA.getAPVTS().getParameter (ParamIDs::bandGain[3]);
                auto* b8 = procA.getAPVTS().getParameter (ParamIDs::bandGain[7]);
                auto* byp = procA.getAPVTS().getParameter (ParamIDs::bypass);

                if (b1  != nullptr) b1->setValueNotifyingHost  (0.2f);
                if (b4  != nullptr) b4->setValueNotifyingHost  (0.7f);
                if (b8  != nullptr) b8->setValueNotifyingHost  (0.9f);
                if (byp != nullptr) byp->setValueNotifyingHost (1.0f);

                procA.getStateInformation (stateBlock);
                expect (stateBlock.getSize() > 0, "state data should be non-empty");
            }

            {
                NESEQAudioProcessor procB;
                procB.setStateInformation (stateBlock.getData(),
                                           static_cast<int> (stateBlock.getSize()));

                auto* b1  = procB.getAPVTS().getParameter (ParamIDs::bandGain[0]);
                auto* b4  = procB.getAPVTS().getParameter (ParamIDs::bandGain[3]);
                auto* b8  = procB.getAPVTS().getParameter (ParamIDs::bandGain[7]);
                auto* byp = procB.getAPVTS().getParameter (ParamIDs::bypass);

                if (b1 != nullptr)
                    expectWithinAbsoluteError (b1->getValue(), 0.2f, 0.01f,
                        "band1_gain should be restored");
                if (b4 != nullptr)
                    expectWithinAbsoluteError (b4->getValue(), 0.7f, 0.01f,
                        "band4_gain should be restored");
                if (b8 != nullptr)
                    expectWithinAbsoluteError (b8->getValue(), 0.9f, 0.01f,
                        "band8_gain should be restored");
                if (byp != nullptr)
                    expectWithinAbsoluteError (byp->getValue(), 1.0f, 0.01f,
                        "bypass should be restored");
            }
        }

        beginTest ("setStateInformation with invalid data is a no-op");
        {
            NESEQAudioProcessor proc;
            auto* b1 = proc.getAPVTS().getParameter (ParamIDs::bandGain[0]);
            const float before = b1 != nullptr ? b1->getValue() : -1.0f;

            const char garbage[] = { 0x00, 0x01, 0x02, 0x03 };
            proc.setStateInformation (garbage, sizeof (garbage));

            if (b1 != nullptr)
                expectEquals (b1->getValue(), before,
                    "parameters should not change after loading invalid state");
        }
    }

    void testProcessMonoLayout()
    {
        beginTest ("processBlock handles mono buffer correctly");
        {
            NESEQAudioProcessor proc;

            juce::AudioProcessor::BusesLayout monoLayout;
            monoLayout.inputBuses .add (juce::AudioChannelSet::mono());
            monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
            proc.setBusesLayout (monoLayout);

            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Boost mid band (band 3, 800 Hz).
            auto* midP = proc.getAPVTS().getParameter (ParamIDs::bandGain[3]);
            if (midP != nullptr) midP->setValueNotifyingHost (0.85f);

            juce::AudioBuffer<float> buffer (1, kTestBlockSize);
            juce::MidiBuffer midi;

            for (int block = 0; block < 64; ++block)
            {
                fillSine (buffer, 800.0f, kTestSampleRate);
                proc.processBlock (buffer, midi);
            }

            const float rms = channelRms (buffer, 0);
            const float inputRms = 1.0f / std::sqrt (2.0f);
            expectGreaterThan (rms, inputRms * 1.5f,
                "mono 800 Hz signal should be boosted with mid gain up");

            proc.releaseResources();
        }
    }
};

static PluginProcessorTests pluginProcessorTests;
} // namespace neseq
