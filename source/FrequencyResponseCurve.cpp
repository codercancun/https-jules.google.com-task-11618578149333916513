#include "FrequencyResponseCurve.h"

namespace neseq
{
FrequencyResponseCurve::FrequencyResponseCurve (
    juce::AudioProcessorValueTreeState& apvtsRef,
    double initialSampleRate)
    : apvts (apvtsRef),
      sampleRate (initialSampleRate)
{
    magnitudesDb.fill (0.0f);
    startTimerHz (30);
}

FrequencyResponseCurve::~FrequencyResponseCurve()
{
    stopTimer();
}

void FrequencyResponseCurve::setSampleRate (double sr)
{
    sampleRate = sr;
}

void FrequencyResponseCurve::timerCallback()
{
    recomputeResponse();
    repaint();
}

void FrequencyResponseCurve::recomputeResponse()
{
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    const float lowDb  = apvts.getRawParameterValue ("low_gain")->load();
    const float midDb  = apvts.getRawParameterValue ("mid_gain")->load();
    const float highDb = apvts.getRawParameterValue ("high_gain")->load();

    const auto lowGain  = juce::Decibels::decibelsToGain (lowDb, -96.0f);
    const auto midGain  = juce::Decibels::decibelsToGain (midDb, -96.0f);
    const auto highGain = juce::Decibels::decibelsToGain (highDb, -96.0f);

    auto lowCoeffs  = Coefficients::makeLowShelf  (sampleRate, ThreeBandEQ::kLowFreqHz,  0.707f, lowGain);
    auto midCoeffs  = Coefficients::makePeakFilter (sampleRate, ThreeBandEQ::kMidFreqHz,  ThreeBandEQ::kMidQ, midGain);
    auto highCoeffs = Coefficients::makeHighShelf  (sampleRate, ThreeBandEQ::kHighFreqHz, 0.707f, highGain);

    const auto logMinFreq = std::log10 (kMinFreqHz);
    const auto logMaxFreq = std::log10 (kMaxFreqHz);

    for (int i = 0; i < kNumPoints; ++i)
    {
        const float normalised = static_cast<float> (i) / static_cast<float> (kNumPoints - 1);
        const float logFreq = logMinFreq + normalised * (logMaxFreq - logMinFreq);
        const double freq = std::pow (10.0, static_cast<double> (logFreq));

        double magLow  = lowCoeffs->getMagnitudeForFrequency  (freq, sampleRate);
        double magMid  = midCoeffs->getMagnitudeForFrequency  (freq, sampleRate);
        double magHigh = highCoeffs->getMagnitudeForFrequency (freq, sampleRate);

        double totalMag = magLow * magMid * magHigh;
        magnitudesDb[static_cast<size_t> (i)] = static_cast<float> (
            juce::Decibels::gainToDecibels (totalMag, -96.0));
    }
}

float FrequencyResponseCurve::freqToX (float freq, float width) const
{
    const auto logMin = std::log10 (kMinFreqHz);
    const auto logMax = std::log10 (kMaxFreqHz);
    const auto logF   = std::log10 (juce::jlimit (kMinFreqHz, kMaxFreqHz, freq));
    return (logF - logMin) / (logMax - logMin) * width;
}

float FrequencyResponseCurve::dbToY (float db, float height) const
{
    const auto clamped = juce::jlimit (kMinDb, kMaxDb, db);
    return (1.0f - (clamped - kMinDb) / (kMaxDb - kMinDb)) * height;
}

void FrequencyResponseCurve::resized()
{
    recomputeResponse();
}

void FrequencyResponseCurve::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto w = bounds.getWidth();
    const auto h = bounds.getHeight();

    // Dark recessed panel background.
    g.setColour (NesPalette::darkGrey);
    g.fillRect (bounds);

    g.setColour (NesPalette::black);
    g.drawRect (bounds, 2.0f);
    g.setColour (NesPalette::grey);
    g.drawRect (bounds.reduced (2.0f), 1.0f);

    const auto plotArea = bounds.reduced (4.0f);
    const auto pw = plotArea.getWidth();
    const auto ph = plotArea.getHeight();
    const auto px = plotArea.getX();
    const auto py = plotArea.getY();

    // Grid lines at key dB values.
    g.setColour (NesPalette::grey.withAlpha (0.25f));
    for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        const float y = py + dbToY (db, ph);
        g.drawHorizontalLine (static_cast<int> (y), px, px + pw);
    }

    // 0 dB reference line (brighter).
    {
        const float y = py + dbToY (0.0f, ph);
        g.setColour (NesPalette::grey.withAlpha (0.5f));
        g.drawHorizontalLine (static_cast<int> (y), px, px + pw);
    }

    // Frequency gridlines at 100, 1k, 10k.
    g.setColour (NesPalette::grey.withAlpha (0.2f));
    for (float freq : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = px + freqToX (freq, pw);
        g.drawVerticalLine (static_cast<int> (x), py, py + ph);
    }

    // Draw the magnitude response curve.
    juce::Path curvePath;
    bool started = false;
    for (int i = 0; i < kNumPoints; ++i)
    {
        const float normX = static_cast<float> (i) / static_cast<float> (kNumPoints - 1);
        const float x = px + normX * pw;
        const float y = py + dbToY (magnitudesDb[static_cast<size_t> (i)], ph);

        if (! started)
        {
            curvePath.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            curvePath.lineTo (x, y);
        }
    }

    // Filled area under curve with semi-transparent colour.
    {
        juce::Path fillPath (curvePath);
        fillPath.lineTo (px + pw, py + ph);
        fillPath.lineTo (px, py + ph);
        fillPath.closeSubPath();

        auto fillColour = NesPalette::cyan.withAlpha (0.15f);
        g.setColour (fillColour);
        g.fillPath (fillPath);
    }

    // Curve stroke.
    g.setColour (NesPalette::cyan);
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));

    // Labels.
    g.setColour (NesPalette::lightGrey.withAlpha (0.6f));
    auto font = juce::Font (juce::Font::getDefaultMonospacedFontName(), 9.0f, juce::Font::plain);
    g.setFont (font);
    g.drawText ("20", static_cast<int> (px + 2), static_cast<int> (py + ph - 12), 20, 12,
                juce::Justification::centredLeft);
    g.drawText ("20k", static_cast<int> (px + pw - 24), static_cast<int> (py + ph - 12), 24, 12,
                juce::Justification::centredRight);
    g.drawText ("+18", static_cast<int> (px + 2), static_cast<int> (py), 26, 12,
                juce::Justification::centredLeft);
    g.drawText ("-18", static_cast<int> (px + 2), static_cast<int> (py + ph - 12), 26, 12,
                juce::Justification::centredLeft);
}
} // namespace neseq
