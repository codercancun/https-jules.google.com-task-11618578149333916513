#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace neseq
{
/**
    Classic NES palette extracted (approximately) from the 2C02 PPU and used
    consistently across the NES-EQ editor to keep the pixel-art look cohesive.
*/
namespace NesPalette
{
    inline const juce::Colour black      { 0xff000000 };
    inline const juce::Colour darkGrey   { 0xff2d2d2d };
    inline const juce::Colour grey       { 0xff7c7c7c };
    inline const juce::Colour lightGrey  { 0xffbcbcbc };
    inline const juce::Colour white      { 0xfffcfcfc };

    inline const juce::Colour red        { 0xffd82800 };
    inline const juce::Colour redShadow  { 0xff9c0000 };
    inline const juce::Colour orange     { 0xfffc7460 };
    inline const juce::Colour yellow     { 0xfff8b800 };
    inline const juce::Colour green      { 0xff00a800 };
    inline const juce::Colour lightGreen { 0xffb8f818 };
    inline const juce::Colour cyan       { 0xff3cbcfc };
    inline const juce::Colour blue       { 0xff0058f8 };
    inline const juce::Colour purple     { 0xff5800e4 };
}

/**
    LookAndFeel for the NES-EQ editor. Provides a chunky pixel-art appearance
    for the few built-in components we still rely on (labels mainly). Custom
    NES-styled controls live in ``NESComponents``.
*/
class NESLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NESLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawLabel (juce::Graphics& g, juce::Label& label) override;

private:
    juce::Typeface::Ptr pixelTypeface;
};
} // namespace neseq
