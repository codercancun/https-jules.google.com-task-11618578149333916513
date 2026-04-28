#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace neseq
{
struct Preset
{
    const char* name;
    float lowDb;
    float midDb;
    float highDb;
    float outputDb;
};

inline constexpr std::array<Preset, 7> kPresets {{
    { "Flat",            0.0f,   0.0f,   0.0f,   0.0f },
    { "Bass Boost",     10.0f,   0.0f,   0.0f,   0.0f },
    { "Vocal Presence",  0.0f,   8.0f,   3.0f,   0.0f },
    { "Treble Bright",   0.0f,   0.0f,  10.0f,   0.0f },
    { "Scoop",          -3.0f,  -8.0f,  -2.0f,   0.0f },
    { "Warm",            5.0f,   2.0f,  -4.0f,   0.0f },
    { "Full Boost",      8.0f,   6.0f,   8.0f,   3.0f },
}};

inline int getNumPresets() { return static_cast<int> (kPresets.size()); }

} // namespace neseq
