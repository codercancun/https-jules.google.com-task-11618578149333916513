#include "SpectrumAnalyzer.h"
#include "PluginProcessor.h"

#include <cmath>

namespace neseq
{
SpectrumAnalyzer::SpectrumAnalyzer (NESEQAudioProcessor& p)
    : audioProcessor (p),
      forwardFFT (NESEQAudioProcessor::getFFTOrder()),
      window (static_cast<size_t> (NESEQAudioProcessor::getFFTSize()),
              juce::dsp::WindowingFunction<float>::hann)
{
    barLevels.fill (0.0f);
    peakLevels.fill (0.0f);
    startTimerHz (30);
}

void SpectrumAnalyzer::timerCallback()
{
    if (! audioProcessor.getFFTReady())
        return;

    auto& fftData = audioProcessor.getFFTData();
    window.multiplyWithWindowingTable (fftData.data(),
                                       static_cast<size_t> (NESEQAudioProcessor::getFFTSize()));
    forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());

    const auto fftSize    = NESEQAudioProcessor::getFFTSize();
    const auto numBins    = fftSize / 2;
    const float binsPerBar = static_cast<float> (numBins) / static_cast<float> (kNumBars);

    for (int bar = 0; bar < kNumBars; ++bar)
    {
        const int startBin = static_cast<int> (std::floor (bar * binsPerBar));
        const int endBin   = juce::jmin (static_cast<int> (std::floor ((bar + 1) * binsPerBar)), numBins);

        float maxVal = 0.0f;
        for (int bin = startBin; bin < endBin; ++bin)
            maxVal = juce::jmax (maxVal, fftData[static_cast<size_t> (bin)]);

        const float level = juce::jlimit (0.0f, 1.0f,
            (juce::Decibels::gainToDecibels (maxVal, -60.0f) + 60.0f) / 60.0f);

        barLevels[static_cast<size_t> (bar)] =
            juce::jmax (level, barLevels[static_cast<size_t> (bar)] * kDecayRate);

        peakLevels[static_cast<size_t> (bar)] =
            juce::jmax (level, peakLevels[static_cast<size_t> (bar)] * kPeakDecay);
    }

    audioProcessor.getFFTReady() = false;
    repaint();
}

void SpectrumAnalyzer::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();

    g.setColour (NesPalette::darkGrey);
    g.fillRect (area);

    g.setColour (NesPalette::black);
    g.drawRect (area, 2);
    g.setColour (NesPalette::grey);
    g.drawRect (area.reduced (2), 2);

    drawBars (g, area.reduced (6));
}

void SpectrumAnalyzer::drawBars (juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    const float barW = static_cast<float> (area.getWidth()) / static_cast<float> (kNumBars);
    const float maxH = static_cast<float> (area.getHeight());

    for (int i = 0; i < kNumBars; ++i)
    {
        const float x = area.getX() + i * barW;
        const float h = barLevels[static_cast<size_t> (i)] * maxH;
        const float y = area.getBottom() - h;

        juce::Colour barColour;
        const float ratio = static_cast<float> (i) / static_cast<float> (kNumBars);
        if (ratio < 0.33f)
            barColour = NesPalette::green;
        else if (ratio < 0.66f)
            barColour = NesPalette::yellow;
        else
            barColour = NesPalette::red;

        g.setColour (barColour);
        g.fillRect (static_cast<int> (x) + 1,
                    static_cast<int> (y),
                    juce::jmax (1, static_cast<int> (barW) - 2),
                    static_cast<int> (h));

        // Peak indicator
        const float peakH = peakLevels[static_cast<size_t> (i)] * maxH;
        const float peakY = area.getBottom() - peakH;
        g.setColour (NesPalette::white);
        g.fillRect (static_cast<int> (x) + 1,
                    static_cast<int> (peakY),
                    juce::jmax (1, static_cast<int> (barW) - 2),
                    2);
    }

    // Grid lines
    g.setColour (NesPalette::grey.withAlpha (0.3f));
    for (int row = 1; row < 4; ++row)
    {
        const int y = area.getY() + (area.getHeight() * row / 4);
        g.drawHorizontalLine (y, static_cast<float> (area.getX()),
                              static_cast<float> (area.getRight()));
    }
}

} // namespace neseq
