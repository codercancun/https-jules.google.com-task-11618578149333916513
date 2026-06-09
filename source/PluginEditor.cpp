#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr int kDefaultWidth  = 560;
constexpr int kDefaultHeight = 420;
} // namespace

NESEQAudioProcessorEditor::NESEQAudioProcessorEditor (NESEQAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("NES-EQ", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, NesPalette::yellow);
    titleLabel.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(),
                                     24.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (lowSlider);
    addAndMakeVisible (midSlider);
    addAndMakeVisible (highSlider);
    addAndMakeVisible (outputSlider);

    addAndMakeVisible (bypassButton);
    addAndMakeVisible (presetSelector);

    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (bypassLabel);

    setResizable (true, true);
    setResizeLimits (420, 300, 1024, 768);
    setSize (kDefaultWidth, kDefaultHeight);
}

NESEQAudioProcessorEditor::~NESEQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void NESEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawBackdrop (g, getLocalBounds());
}

void NESEQAudioProcessorEditor::drawBackdrop (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.fillAll (NesPalette::black);

    g.setColour (juce::Colour (0xff101018));
    for (int y = 0; y < area.getHeight(); y += 4)
        g.fillRect (area.getX(), y, area.getWidth(), 1);

    drawTitleBar (g, area.removeFromTop (46));

    auto frame = getLocalBounds();
    g.setColour (NesPalette::white);
    g.drawRect (frame, 2);
    g.setColour (NesPalette::grey);
    g.drawRect (frame.reduced (2), 2);
}

void NESEQAudioProcessorEditor::drawTitleBar (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (NesPalette::red);
    g.fillRect (area);

    g.setColour (NesPalette::redShadow);
    g.fillRect (area.removeFromBottom (4));

    juce::Random rng (0x4AE5);
    g.setColour (NesPalette::yellow);
    for (int i = 0; i < 14; ++i)
    {
        const auto x = rng.nextInt (area.getWidth());
        const auto y = rng.nextInt (area.getHeight());
        g.fillRect (area.getX() + x, area.getY() + y, 2, 2);
    }
}

void NESEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    titleLabel.setBounds (area.removeFromTop (40));
    area.removeFromTop (8);

    // Preset selector strip.
    presetSelector.setBounds (area.removeFromBottom (28).reduced (8, 0));
    area.removeFromBottom (4);

    // Bottom control strip: bypass button + label.
    auto bottom = area.removeFromBottom (80);
    auto bypassArea = bottom.removeFromRight (140).reduced (10);
    bypassLabel.setBounds (bypassArea.removeFromBottom (18));
    const auto buttonSize = juce::jmin (bypassArea.getWidth(), bypassArea.getHeight());
    auto buttonBounds = bypassArea.withSizeKeepingCentre (buttonSize, buttonSize);
    bypassButton.setBounds (buttonBounds);

    // Four band sliders evenly across the remainder.
    auto sliderStrip = area.reduced (8);
    const auto sliderW = sliderStrip.getWidth() / 4;
    lowSlider   .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (6, 0));
    midSlider   .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (6, 0));
    highSlider  .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (6, 0));
    outputSlider.setBounds (sliderStrip.reduced (6, 0));
}
} // namespace neseq
