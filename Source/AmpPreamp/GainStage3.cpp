#include "GainStage3.h"
#include <cmath>
#include <juce_core/juce_core.h>

void GainStage3::prepare(double sampleRate)
{
    // store sample rate later if needed
}

float GainStage3::processSample(float x, float drive)
{
    if (drive <= 0.05f)
        return x;

    const auto drive01 = juce::jlimit(0.0f, 1.0f, drive / 10.0f);
    const auto shaped = std::pow(drive01, 1.15f);

    const auto gain = juce::jmap(shaped, 1.0f, 70.0f);

    x *= gain;

    const auto sharp = std::tanh(x * 2.2f);

    const auto hard = juce::jlimit(-1.0f, 1.0f, x);

    const auto hardMix = juce::jmap(shaped, 0.1f, 0.35f);

    return sharp + hardMix * (hard - sharp);
}
