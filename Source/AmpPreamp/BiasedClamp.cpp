#include "BiasedClamp.h"

void BiasedClamp::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    channelCount = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(channelCount);

    preHighPass.prepare(spec);
    postLowPass.prepare(spec);
    outputDcBlocker.prepare(spec);

    updateFilters();
    reset();
}

void BiasedClamp::reset()
{
    preHighPass.reset();
    postLowPass.reset();
    outputDcBlocker.reset();
}

void BiasedClamp::process(juce::AudioBuffer<float> &buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    if (numSamples == 0)
        return;

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        preHighPass.process(context);
    }

    const float inputTrim = juce::Decibels::decibelsToGain(inputTrimDb);
    const float outputTrim = juce::Decibels::decibelsToGain(outputTrimDb);

    for (int channel = 0; channel < numChannels; ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float x = samples[sample];

            x *= inputTrim;

            x = shapeSample(x, bias, positiveLimit, negativeLimit, knee);

            x *= outputTrim;

            samples[sample] = x;
        }
    }

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        postLowPass.process(context);
        outputDcBlocker.process(context);
    }
}

float BiasedClamp::limitWithKnee(float x, float lowerLimit, float upperLimit, float kneeAmount)
{
    jassert(lowerLimit < upperLimit);

    const float safeKnee = juce::jlimit(0.0001f, 1.0f, kneeAmount);

    if (x > upperLimit) {
        const float excess = x - upperLimit;

        // Limiter-style soft knee: contains only the excess above the threshold.
        return upperLimit + (excess / (1.0f + excess / safeKnee));
    }

    if (x < lowerLimit) {
        const float excess = lowerLimit - x;

        return lowerLimit - (excess / (1.0f + excess / safeKnee));
    }

    return x;
}

float BiasedClamp::shapeSample(
    float x, float clampBias, float posLimit, float negLimit, float kneeAmount)
{
    const float lowerLimit = juce::jmax(0.001f, negLimit);
    const float upperLimit = juce::jmax(0.001f, posLimit);

    const float biased = x + clampBias;

    const float limited = limitWithKnee(biased, lowerLimit, upperLimit, kneeAmount);

    // Remove the static offset caused by the bias so the audio path remains centred.
    const float zeroPoint = limitWithKnee(clampBias, lowerLimit, upperLimit, kneeAmount);

    return limited - zeroPoint;
}

void BiasedClamp::setInputTrimDb(float newInputTrimDb)
{
    inputTrimDb = juce::jlimit(-36.0f, 36.0f, newInputTrimDb);
}

void BiasedClamp::setBias(float newBias)
{
    bias = juce::jlimit(-2.0f, 2.0f, newBias);
}

void BiasedClamp::setPositiveLimit(float newPositiveLimit)
{
    positiveLimit = juce::jlimit(0.001f, 4.0f, newPositiveLimit);
}

void BiasedClamp::setNegativeLimit(float newNegativeLimit)
{
    negativeLimit = juce::jlimit(0.001f, 4.0f, newNegativeLimit);
}

void BiasedClamp::setKnee(float newKnee)
{
    knee = juce::jlimit(0.0001f, 1.0f, newKnee);
}

void BiasedClamp::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 24.0f, newOutputTrimDb);
}

void BiasedClamp::setPreHighPassHz(float newFrequencyHz)
{
    preHighPassHz = juce::jlimit(5.0f, 1000.0f, newFrequencyHz);
    updateFilters();
}

void BiasedClamp::setPostLowPassHz(float newFrequencyHz)
{
    postLowPassHz = juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), newFrequencyHz);

    updateFilters();
}

void BiasedClamp::updateFilters()
{
    const auto safePostLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), postLowPassHz);

    *preHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, preHighPassHz);

    *postLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safePostLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}
