#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr int kDefaultWidth  = 620;
constexpr int kDefaultHeight = 520;
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

    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (bypassLabel);

    // Preset selector
    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType (juce::Justification::centred);
    presetLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (presetLabel);

    for (int i = 0; i < getNumPresets(); ++i)
        presetSelector.addItem (kPresets[static_cast<size_t> (i)].name, i + 1);

    presetSelector.setSelectedId (0, juce::dontSendNotification);
    presetSelector.setTextWhenNothingSelected ("---");
    presetSelector.setColour (juce::ComboBox::backgroundColourId, NesPalette::darkGrey);
    presetSelector.setColour (juce::ComboBox::textColourId, NesPalette::yellow);
    presetSelector.setColour (juce::ComboBox::outlineColourId, NesPalette::grey);
    presetSelector.setColour (juce::ComboBox::arrowColourId, NesPalette::white);
    presetSelector.onChange = [this]
    {
        const int idx = presetSelector.getSelectedId() - 1;
        if (idx >= 0 && idx < getNumPresets())
            applyPreset (idx);
    };
    addAndMakeVisible (presetSelector);

    addAndMakeVisible (analyzer);

    setResizable (true, true);
    setResizeLimits (520, 420, 1280, 960);
    setSize (kDefaultWidth, kDefaultHeight);
}

NESEQAudioProcessorEditor::~NESEQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void NESEQAudioProcessorEditor::applyPreset (int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= getNumPresets())
        return;

    const auto& preset = kPresets[static_cast<size_t> (presetIndex)];
    auto& apvts = audioProcessor.getAPVTS();

    if (auto* p = apvts.getParameter (ParamIDs::lowGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.lowDb));
    if (auto* p = apvts.getParameter (ParamIDs::midGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.midDb));
    if (auto* p = apvts.getParameter (ParamIDs::highGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.highDb));
    if (auto* p = apvts.getParameter (ParamIDs::outputGain))
        p->setValueNotifyingHost (p->convertTo0to1 (preset.outputDb));
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

    // Title banner
    titleLabel.setBounds (area.removeFromTop (40));
    area.removeFromTop (4);

    // Preset row
    auto presetRow = area.removeFromTop (30);
    presetLabel.setBounds (presetRow.removeFromLeft (64));
    presetSelector.setBounds (presetRow.reduced (2));
    area.removeFromTop (4);

    // Bottom control strip: bypass button + label
    auto bottom = area.removeFromBottom (80);
    auto bypassArea = bottom.removeFromRight (100).reduced (8);
    bypassLabel.setBounds (bypassArea.removeFromBottom (18));
    const auto buttonSize = juce::jmin (bypassArea.getWidth(), bypassArea.getHeight());
    auto buttonBounds = bypassArea.withSizeKeepingCentre (buttonSize, buttonSize);
    bypassButton.setBounds (buttonBounds);

    // Spectrum analyzer in the bottom-left
    analyzer.setBounds (bottom.reduced (4));

    // EQ sliders: low, mid, high, output – evenly across the main area
    auto sliderStrip = area.reduced (4);
    const auto sliderW = sliderStrip.getWidth() / 4;
    lowSlider   .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (4, 0));
    midSlider   .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (4, 0));
    highSlider  .setBounds (sliderStrip.removeFromLeft (sliderW).reduced (4, 0));
    outputSlider.setBounds (sliderStrip.reduced (4, 0));
}
} // namespace neseq
