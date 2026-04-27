#include "PluginEditor.h"

namespace neseq
{
namespace
{
constexpr int kDefaultWidth  = 520;
constexpr int kDefaultHeight = 480;

struct Preset
{
    const char* name;
    float lowDb;
    float midDb;
    float highDb;
};

constexpr Preset kPresets[] =
{
    { "Flat",             0.0f,   0.0f,   0.0f },
    { "Bass Boost",      10.0f,   0.0f,   0.0f },
    { "Bass Cut",       -10.0f,   0.0f,   0.0f },
    { "Vocal Presence",   0.0f,   8.0f,   3.0f },
    { "Treble Boost",     0.0f,   0.0f,  10.0f },
    { "Scoop (V-shape)", 6.0f,  -6.0f,   6.0f },
    { "Mid Boost",        0.0f,  10.0f,   0.0f },
    { "Warm",             5.0f,  -2.0f,  -4.0f },
    { "Bright",          -3.0f,   2.0f,   8.0f },
    { "Lo-Fi NES",        8.0f,  -4.0f, -10.0f },
};

constexpr int kNumPresets = static_cast<int> (sizeof (kPresets) / sizeof (kPresets[0]));
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

    addAndMakeVisible (bypassButton);

    bypassLabel.setText ("BYPASS", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, NesPalette::white);
    addAndMakeVisible (bypassLabel);

    addAndMakeVisible (responseCurve);

    populatePresetMenu();
    presetSelector.setColour (juce::ComboBox::backgroundColourId, NesPalette::darkGrey);
    presetSelector.setColour (juce::ComboBox::textColourId, NesPalette::white);
    presetSelector.setColour (juce::ComboBox::outlineColourId, NesPalette::grey);
    presetSelector.setColour (juce::ComboBox::arrowColourId, NesPalette::yellow);
    presetSelector.setTextWhenNothingSelected ("-- Preset --");
    presetSelector.onChange = [this]
    {
        const int idx = presetSelector.getSelectedId() - 1;
        if (idx >= 0 && idx < kNumPresets)
            applyPreset (idx);
    };
    addAndMakeVisible (presetSelector);

    setResizable (true, true);
    setResizeLimits (400, 400, 1200, 900);
    setSize (kDefaultWidth, kDefaultHeight);
}

NESEQAudioProcessorEditor::~NESEQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void NESEQAudioProcessorEditor::populatePresetMenu()
{
    presetSelector.clear();
    for (int i = 0; i < kNumPresets; ++i)
        presetSelector.addItem (kPresets[i].name, i + 1);
}

void NESEQAudioProcessorEditor::applyPreset (int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= kNumPresets)
        return;

    const auto& preset = kPresets[presetIndex];
    auto& apvts = audioProcessor.getAPVTS();

    if (auto* param = apvts.getParameter (ParamIDs::lowGain))
        param->setValueNotifyingHost (param->convertTo0to1 (preset.lowDb));
    if (auto* param = apvts.getParameter (ParamIDs::midGain))
        param->setValueNotifyingHost (param->convertTo0to1 (preset.midDb));
    if (auto* param = apvts.getParameter (ParamIDs::highGain))
        param->setValueNotifyingHost (param->convertTo0to1 (preset.highDb));
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
    area.removeFromTop (4);

    // Preset selector row.
    auto presetRow = area.removeFromTop (28);
    presetSelector.setBounds (presetRow.reduced (40, 0));
    area.removeFromTop (6);

    // Frequency response visualizer.
    responseCurve.setBounds (area.removeFromTop (100).reduced (8, 0));
    area.removeFromTop (6);

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
