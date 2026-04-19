#include <gtest/gtest.h>
#include "../VintageEqAudioProcessor.h"

TEST(VintageEqAudioProcessorTest, SupportsMonoAndStereoLayouts) {
    VintageEqAudioProcessor processor;

    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.inputBuses.add(juce::AudioChannelSet::mono());
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());
    EXPECT_TRUE(processor.isBusesLayoutSupported(monoLayout));

    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
    EXPECT_TRUE(processor.isBusesLayoutSupported(stereoLayout));
}

TEST(VintageEqAudioProcessorTest, RejectsOtherLayouts) {
    VintageEqAudioProcessor processor;

    juce::AudioProcessor::BusesLayout surroundLayout;
    surroundLayout.inputBuses.add(juce::AudioChannelSet::create5point1());
    surroundLayout.outputBuses.add(juce::AudioChannelSet::create5point1());
    EXPECT_FALSE(processor.isBusesLayoutSupported(surroundLayout));

    juce::AudioProcessor::BusesLayout mismatchLayout;
    mismatchLayout.inputBuses.add(juce::AudioChannelSet::mono());
    mismatchLayout.outputBuses.add(juce::AudioChannelSet::stereo());
    EXPECT_FALSE(processor.isBusesLayoutSupported(mismatchLayout));
}
