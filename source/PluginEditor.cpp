#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr int kDefaultWidth  = 780;
constexpr int kDefaultHeight = 400;
} // namespace

NESEQAudioProcessorEditor::NESEQAudioProcessorEditor (NESEQAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("NES-EQ  8-BAND", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, NesPalette::yellow);
    titleLabel.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(),
                                     24.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < EightBandEQ::kNumBands; ++i)
    {
        bandSliders[static_cast<size_t> (i)] = std::make_unique<PowerMeterSlider> (
            audioProcessor.getAPVTS(),
            ParamIDs::bandGain[static_cast<size_t> (i)],
            EightBandEQ::kLabels[static_cast<size_t> (i)]);
        addAndMakeVisible (*bandSliders[static_cast<size_t> (i)]);
    }

    addAndMakeVisible (bypassButton);

    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (bypassLabel);

    setResizable (true, true);
    setResizeLimits (640, 320, 1280, 800);
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

    // Scanline-style horizontal pixel pattern.
    g.setColour (juce::Colour (0xff101018));
    for (int y = 0; y < area.getHeight(); y += 4)
        g.fillRect (area.getX(), y, area.getWidth(), 1);

    drawTitleBar (g, area.removeFromTop (46));

    // Chunky pixel frame.
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

    // Pixel stars scattered across the banner.
    juce::Random rng (0x4AE5);
    g.setColour (NesPalette::yellow);
    for (int i = 0; i < 20; ++i)
    {
        const auto x = rng.nextInt (area.getWidth());
        const auto y = rng.nextInt (area.getHeight());
        g.fillRect (area.getX() + x, area.getY() + y, 2, 2);
    }
}

void NESEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    // Title banner
    titleLabel.setBounds (area.removeFromTop (40));
    area.removeFromTop (4);

    // Bottom control strip: bypass button + label.
    auto bottom = area.removeFromBottom (70);
    auto bypassArea = bottom.withSizeKeepingCentre (100, 70);
    bypassLabel.setBounds (bypassArea.removeFromBottom (18));
    const auto buttonSize = juce::jmin (bypassArea.getWidth(), bypassArea.getHeight());
    auto buttonBounds = bypassArea.withSizeKeepingCentre (buttonSize, buttonSize);
    bypassButton.setBounds (buttonBounds);

    // Eight band sliders evenly across the remainder.
    auto sliderStrip = area.reduced (4);
    const auto sliderW = sliderStrip.getWidth() / EightBandEQ::kNumBands;
    for (int i = 0; i < EightBandEQ::kNumBands; ++i)
        bandSliders[static_cast<size_t> (i)]->setBounds (
            sliderStrip.removeFromLeft (sliderW).reduced (3, 0));
}
} // namespace neseq
