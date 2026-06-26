#include "GainStage1.h"
#include <cmath>
#include <juce_core/juce_core.h>

void GainStage1::prepare(double sampleRate)
{
    // store sample rate later if needed
}

float GainStage1::processSample(float x, float gain, float bias)
{
    if (gain <= 0.05f)
        return x;

    const auto drive01 = juce::jlimit(0.0f, 1.0f, gain / 10.0f);
    const auto shaped = std::pow(drive01, 1.5f);

    const auto drive = juce::jmap(shaped, 1.0f, 30.0f);

    x *= drive;

    if (x >= 0.0f)
        x = std::tanh(x * (1.0f + bias));

    return std::tanh(x * (1.0f - bias));
}
