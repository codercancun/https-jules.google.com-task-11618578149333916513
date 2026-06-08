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

/** Fill a buffer with a sine tone at the given frequency. */
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

/** Return the RMS of a single channel in a buffer. */
float channelRms (const juce::AudioBuffer<float>& buffer, int channel)
{
    double sumSq = 0.0;
    const auto* data = buffer.getReadPointer (channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        sumSq += static_cast<double> (data[i]) * data[i];
    return static_cast<float> (std::sqrt (sumSq / buffer.getNumSamples()));
}
} // namespace

// =========================================================================
// PluginProcessor unit tests
// =========================================================================

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
    // -----------------------------------------------------------------
    void testParameterLayout()
    {
        beginTest ("parameter layout has correct IDs, ranges, and defaults");
        {
            NESEQAudioProcessor proc;
            auto& apvts = proc.getAPVTS();

            auto* low  = apvts.getParameter (ParamIDs::lowGain);
            auto* mid  = apvts.getParameter (ParamIDs::midGain);
            auto* high = apvts.getParameter (ParamIDs::highGain);
            auto* byp  = apvts.getParameter (ParamIDs::bypass);

            expect (low  != nullptr, "low_gain parameter should exist");
            expect (mid  != nullptr, "mid_gain parameter should exist");
            expect (high != nullptr, "high_gain parameter should exist");
            expect (byp  != nullptr, "bypass parameter should exist");

            if (low == nullptr || mid == nullptr || high == nullptr || byp == nullptr)
                return;

            // Default values: gain params default to 0 dB, bypass to off (0).
            expectWithinAbsoluteError (low->getDefaultValue(),  0.5f, 0.01f,
                "low_gain default normalised should map to 0 dB (midpoint)");
            expectWithinAbsoluteError (mid->getDefaultValue(),  0.5f, 0.01f,
                "mid_gain default normalised should map to 0 dB (midpoint)");
            expectWithinAbsoluteError (high->getDefaultValue(), 0.5f, 0.01f,
                "high_gain default normalised should map to 0 dB (midpoint)");
            expectWithinAbsoluteError (byp->getDefaultValue(),  0.0f, 0.01f,
                "bypass default normalised should be 0 (off)");

            // Gain param range: -15 to +15 dB. Denormalised min/max.
            auto* lowFloat = dynamic_cast<juce::AudioParameterFloat*> (low);
            expect (lowFloat != nullptr, "low_gain should be AudioParameterFloat");
            if (lowFloat != nullptr)
            {
                const auto& range = lowFloat->getNormalisableRange();
                expectWithinAbsoluteError (range.start, -15.0f, 0.01f,
                    "low_gain range start should be -15 dB");
                expectWithinAbsoluteError (range.end, 15.0f, 0.01f,
                    "low_gain range end should be +15 dB");
            }
        }
    }

    // -----------------------------------------------------------------
    void testBusLayouts()
    {
        beginTest ("bus layout: stereo in/out accepted");
        {
            NESEQAudioProcessor proc;
            juce::AudioProcessor::BusesLayout stereoLayout;
            stereoLayout.inputBuses .add (juce::AudioChannelSet::stereo());
            stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (proc.isBusesLayoutSupported (stereoLayout),
                "stereo in -> stereo out should be supported");
        }

        beginTest ("bus layout: mono in/out accepted");
        {
            NESEQAudioProcessor proc;
            juce::AudioProcessor::BusesLayout monoLayout;
            monoLayout.inputBuses .add (juce::AudioChannelSet::mono());
            monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
            expect (proc.isBusesLayoutSupported (monoLayout),
                "mono in -> mono out should be supported");
        }

        beginTest ("bus layout: mismatched in/out rejected");
        {
            NESEQAudioProcessor proc;
            juce::AudioProcessor::BusesLayout mismatch;
            mismatch.inputBuses .add (juce::AudioChannelSet::mono());
            mismatch.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (! proc.isBusesLayoutSupported (mismatch),
                "mono in -> stereo out should be rejected");
        }

        beginTest ("bus layout: surround rejected");
        {
            NESEQAudioProcessor proc;
            juce::AudioProcessor::BusesLayout surround;
            surround.inputBuses .add (juce::AudioChannelSet::create5point1());
            surround.outputBuses.add (juce::AudioChannelSet::create5point1());
            expect (! proc.isBusesLayoutSupported (surround),
                "5.1 surround should be rejected");
        }
    }

    // -----------------------------------------------------------------
    void testBypassPassesThrough()
    {
        beginTest ("processBlock with bypass produces unmodified output");
        {
            NESEQAudioProcessor proc;
            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Set a non-zero EQ gain so we can tell bypass actually skips DSP.
            auto* lowParam = proc.getAPVTS().getParameter (ParamIDs::lowGain);
            if (lowParam != nullptr)
                lowParam->setValueNotifyingHost (0.8f); // ~+9 dB

            // Enable bypass.
            auto* byp = proc.getAPVTS().getParameter (ParamIDs::bypass);
            if (byp != nullptr)
                byp->setValueNotifyingHost (1.0f);

            juce::AudioBuffer<float> buffer (2, kTestBlockSize);
            fillSine (buffer, 200.0f, kTestSampleRate);

            // Keep a copy of the input.
            juce::AudioBuffer<float> reference (buffer);

            juce::MidiBuffer midi;
            proc.processBlock (buffer, midi);

            // Output should exactly equal the input when bypassed.
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

    // -----------------------------------------------------------------
    void testProcessBlockAppliesEQ()
    {
        beginTest ("processBlock applies EQ when not bypassed");
        {
            NESEQAudioProcessor proc;
            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Boost the low band heavily.
            auto* lowParam = proc.getAPVTS().getParameter (ParamIDs::lowGain);
            if (lowParam != nullptr)
                lowParam->setValueNotifyingHost (0.9f); // ~+12 dB

            // Ensure bypass is off.
            auto* byp = proc.getAPVTS().getParameter (ParamIDs::bypass);
            if (byp != nullptr)
                byp->setValueNotifyingHost (0.0f);

            // Run enough blocks to reach steady-state.
            juce::AudioBuffer<float> buffer (2, kTestBlockSize);
            juce::MidiBuffer midi;
            float lastRmsL = 0.0f;

            for (int block = 0; block < 64; ++block)
            {
                fillSine (buffer, 50.0f, kTestSampleRate);
                proc.processBlock (buffer, midi);
                if (block >= 32)
                    lastRmsL = channelRms (buffer, 0);
            }

            const float inputRms = 1.0f / std::sqrt (2.0f);
            expectGreaterThan (lastRmsL, inputRms * 2.0f,
                "low-frequency signal should be significantly boosted");

            // Both channels should be processed.
            fillSine (buffer, 50.0f, kTestSampleRate);
            proc.processBlock (buffer, midi);
            const float rmsL = channelRms (buffer, 0);
            const float rmsR = channelRms (buffer, 1);
            expectWithinAbsoluteError (rmsL, rmsR, 0.01f,
                "left and right channels should receive identical EQ");

            proc.releaseResources();
        }
    }

    // -----------------------------------------------------------------
    void testStateRoundTrip()
    {
        beginTest ("state save / restore preserves parameter values");
        {
            juce::MemoryBlock stateBlock;

            // Create a processor, set some non-default values, and save state.
            {
                NESEQAudioProcessor procA;
                auto* lowP  = procA.getAPVTS().getParameter (ParamIDs::lowGain);
                auto* midP  = procA.getAPVTS().getParameter (ParamIDs::midGain);
                auto* highP = procA.getAPVTS().getParameter (ParamIDs::highGain);
                auto* bypP  = procA.getAPVTS().getParameter (ParamIDs::bypass);

                if (lowP  != nullptr) lowP->setValueNotifyingHost  (0.2f);
                if (midP  != nullptr) midP->setValueNotifyingHost  (0.7f);
                if (highP != nullptr) highP->setValueNotifyingHost (0.9f);
                if (bypP  != nullptr) bypP->setValueNotifyingHost  (1.0f);

                procA.getStateInformation (stateBlock);
                expect (stateBlock.getSize() > 0, "state data should be non-empty");
            }

            // Create a fresh processor and restore the saved state.
            {
                NESEQAudioProcessor procB;
                procB.setStateInformation (stateBlock.getData(),
                                           static_cast<int> (stateBlock.getSize()));

                auto* lowP  = procB.getAPVTS().getParameter (ParamIDs::lowGain);
                auto* midP  = procB.getAPVTS().getParameter (ParamIDs::midGain);
                auto* highP = procB.getAPVTS().getParameter (ParamIDs::highGain);
                auto* bypP  = procB.getAPVTS().getParameter (ParamIDs::bypass);

                if (lowP != nullptr)
                    expectWithinAbsoluteError (lowP->getValue(), 0.2f, 0.01f,
                        "low_gain should be restored");
                if (midP != nullptr)
                    expectWithinAbsoluteError (midP->getValue(), 0.7f, 0.01f,
                        "mid_gain should be restored");
                if (highP != nullptr)
                    expectWithinAbsoluteError (highP->getValue(), 0.9f, 0.01f,
                        "high_gain should be restored");
                if (bypP != nullptr)
                    expectWithinAbsoluteError (bypP->getValue(), 1.0f, 0.01f,
                        "bypass should be restored");
            }
        }

        beginTest ("setStateInformation with invalid data is a no-op");
        {
            NESEQAudioProcessor proc;
            auto* lowP = proc.getAPVTS().getParameter (ParamIDs::lowGain);
            const float before = lowP != nullptr ? lowP->getValue() : -1.0f;

            // Feed garbage bytes.
            const char garbage[] = { 0x00, 0x01, 0x02, 0x03 };
            proc.setStateInformation (garbage, sizeof (garbage));

            if (lowP != nullptr)
                expectEquals (lowP->getValue(), before,
                    "parameters should not change after loading invalid state");
        }
    }

    // -----------------------------------------------------------------
    void testProcessMonoLayout()
    {
        beginTest ("processBlock handles mono buffer correctly");
        {
            NESEQAudioProcessor proc;

            // Switch bus layout to mono in / mono out before preparing.
            juce::AudioProcessor::BusesLayout monoLayout;
            monoLayout.inputBuses .add (juce::AudioChannelSet::mono());
            monoLayout.outputBuses.add (juce::AudioChannelSet::mono());
            proc.setBusesLayout (monoLayout);

            proc.prepareToPlay (kTestSampleRate, kTestBlockSize);

            // Boost mid band.
            auto* midP = proc.getAPVTS().getParameter (ParamIDs::midGain);
            if (midP != nullptr)
                midP->setValueNotifyingHost (0.85f);

            juce::AudioBuffer<float> buffer (1, kTestBlockSize);
            juce::MidiBuffer midi;

            for (int block = 0; block < 64; ++block)
            {
                fillSine (buffer, 1000.0f, kTestSampleRate);
                proc.processBlock (buffer, midi);
            }

            const float rms = channelRms (buffer, 0);
            const float inputRms = 1.0f / std::sqrt (2.0f);
            expectGreaterThan (rms, inputRms * 1.5f,
                "mono 1 kHz signal should be boosted with mid gain up");

            proc.releaseResources();
        }
    }
};

static PluginProcessorTests pluginProcessorTests;
} // namespace neseq
