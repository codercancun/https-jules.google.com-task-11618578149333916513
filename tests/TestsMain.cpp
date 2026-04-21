#include <juce_core/juce_core.h>

#include <iostream>

int main (int, char**)
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int numFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (auto* r = runner.getResult (i))
            numFailures += r->failures;
    }

    if (numFailures > 0)
    {
        std::cerr << "NES-EQ tests: " << numFailures << " failure(s)\n";
        return 1;
    }

    std::cout << "NES-EQ tests: all passed\n";
    return 0;
}
