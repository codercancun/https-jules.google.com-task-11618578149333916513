#include "PluginEditor.h"
#include "NESDrawUtils.h"

namespace neseq
{
namespace
{
constexpr int kDefaultWidth  = 480;
constexpr int kDefaultHeight = 360;
} // namespace

NESEQAudioProcessorEditor::NESEQAudioProcessorEditor (NESEQAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("NES-EQ", juce::dontSendNotification);
    titleLabel.setFont (DrawUtils::makeNESFont (24.0f));
    DrawUtils::configureLabel (titleLabel, *this, NesPalette::yellow);

    addAndMakeVisible (lowSlider);
    addAndMakeVisible (midSlider);
    addAndMakeVisible (highSlider);

    addAndMakeVisible (bypassButton);

    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    DrawUtils::configureLabel (bypassLabel, *this, NesPalette::white);

    setResizable (true, true);
    setResizeLimits (360, 280, 1024, 768);
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
    // Deep-space backdrop reminiscent of the NES title-screen gradient.
    g.fillAll (NesPalette::black);

    // Scanline-style horizontal pixel pattern to reinforce the CRT / 8-bit look.
    g.setColour (juce::Colour (0xff101018));
    for (int y = 0; y < area.getHeight(); y += 4)
        g.fillRect (area.getX(), y, area.getWidth(), 1);

    drawTitleBar (g, area.removeFromTop (46));

    // Chunky pixel frame around the plugin.
    DrawUtils::drawPixelBorder (g, getLocalBounds(), NesPalette::white, NesPalette::grey);
}

void NESEQAudioProcessorEditor::drawTitleBar (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Red banner across the top, classic Mario-title-screen vibe.
    g.setColour (NesPalette::red);
    g.fillRect (area);

    g.setColour (NesPalette::redShadow);
    g.fillRect (area.removeFromBottom (4));

    // Pixel stars scattered across the banner.
    juce::Random rng (0x4AE5); // deterministic layout
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

    // Title banner
    titleLabel.setBounds (area.removeFromTop (40));

    area.removeFromTop (8);

    // Bottom control strip: bypass button + label.
    auto bottom = area.removeFromBottom (80);
    auto bypassArea = bottom.removeFromRight (140).reduced (10);
    bypassLabel.setBounds (bypassArea.removeFromBottom (18));
    const auto buttonSize = juce::jmin (bypassArea.getWidth(), bypassArea.getHeight());
    auto buttonBounds = bypassArea.withSizeKeepingCentre (buttonSize, buttonSize);
    bypassButton.setBounds (buttonBounds);

    // Three band sliders evenly across the remainder.
    auto sliderStrip = area.reduced (8);
    const auto sliderW = sliderStrip.getWidth() / 3;
    lowSlider .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (6, 0));
    midSlider .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (6, 0));
    highSlider.setBounds (sliderStrip.reduced (6, 0));
}
} // namespace neseq
