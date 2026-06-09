#pragma once

#include <array>

namespace neseq
{
struct Preset
{
    const char* name;
    float lowDb;
    float midDb;
    float highDb;
};

inline constexpr std::array<Preset, 6> kFactoryPresets {{
    { "Flat",        0.0f,   0.0f,   0.0f },
    { "Vocal Boost", -2.0f,  4.5f,   2.0f },
    { "Bass Heavy",  8.0f,  -1.0f,  -2.0f },
    { "Bright",     -1.0f,   0.0f,   6.0f },
    { "Warm",        3.0f,   1.0f,  -4.0f },
    { "Scoop",      3.0f,  -5.0f,   3.0f },
}};
} // namespace neseq
