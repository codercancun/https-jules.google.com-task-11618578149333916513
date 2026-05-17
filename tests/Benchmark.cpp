#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <iostream>
#include <chrono>

int main()
{
    const int iterations = 100000;

    juce::MessageManager::getInstance(); // Initialize JUCE messaging if needed

    // Baseline
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        juce::Font font(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold);
        juce::ignoreUnused(font);
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // Optimized
    auto start2 = std::chrono::high_resolution_clock::now();
    juce::Font cachedFont(juce::Font::getDefaultMonospacedFontName(), 20.0f, juce::Font::bold);
    for (int i = 0; i < iterations; ++i) {
        juce::Font font = cachedFont;
        juce::ignoreUnused(font);
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();

    std::cout << "Baseline (Repeated Instantiation): " << duration1 << " us\n";
    std::cout << "Optimized (Cached Font): " << duration2 << " us\n";

    juce::MessageManager::deleteInstance();
    return 0;
}
