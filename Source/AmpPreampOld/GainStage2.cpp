#include "GainStage2.h"
#include <cmath>
#include <juce_core/juce_core.h>

void GainStage2::prepare(double sampleRate)
{
    // store sample rate later if needed
}

float GainStage2::processSample(float x, float drive)
{
    if (drive <= 0.05f)
        return x;

    const auto drive01 = juce::jlimit(0.0f, 1.0f, drive / 10.0f);
    const auto shaped = std::pow(drive01, 1.2f);

    const auto gain = juce::jmap(shaped, 1.0f, 90.0f);

    x *= gain;

    const auto soft = (2.0f / juce::MathConstants<float>::pi) * std::atan(x);

    const auto hard = juce::jlimit(-1.0f, 1.0f, x);

    const auto hardMix = juce::jmap(shaped, 0.2f, 0.75f);

    return soft + hardMix * (hard - soft);
}
