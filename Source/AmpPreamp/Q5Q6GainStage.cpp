#include "Q5Q6GainStage.h"

void Q5Q6GainStage::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
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

void Q5Q6GainStage::reset()
{
    preHighPass.reset();
    postLowPass.reset();
    outputDcBlocker.reset();
}

void Q5Q6GainStage::process(juce::AudioBuffer<float> &buffer)
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

    const float drive = juce::Decibels::decibelsToGain(driveDb);
    const float outputTrim = juce::Decibels::decibelsToGain(outputTrimDb);

    for (int channel = 0; channel < numChannels; ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float x = samples[sample];

            x *= drive;

            x = shapeSample(x, bias, asymmetry);

            // Common-emitter-ish stage: collector output is inverted.
            // Absolute polarity is not vital for tone, but it helps when comparing
            // against LTspice block renders.
            x = -x;

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

static float limitWithKnee(float x, float lowerLimit, float upperLimit, float knee)
{
    knee = juce::jlimit(0.0001f, 1.0f, knee);

    if (x > upperLimit) {
        const float excess = x - upperLimit;
        return upperLimit + (excess / (1.0f + excess / knee));
    }

    if (x < lowerLimit) {
        const float excess = lowerLimit - x;
        return lowerLimit - (excess / (1.0f + excess / knee));
    }

    return x;
}

float Q5Q6GainStage::shapeSample(float x, float shapeBias, float shapeAsymmetry)
{
    const float biased = x + shapeBias;

    const float upperLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 1.2f, 1.8f);

    const float lowerLimit = -juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 1.2f, 0.65f);

    constexpr float knee = 0.08f;

    const float limited = limitWithKnee(biased, lowerLimit, upperLimit, knee);
    const float zeroPoint = limitWithKnee(shapeBias, lowerLimit, upperLimit, knee);

    return limited - zeroPoint;
}

void Q5Q6GainStage::setDriveDb(float newDriveDb)
{
    driveDb = juce::jlimit(0.0f, 48.0f, newDriveDb);
}

void Q5Q6GainStage::setBias(float newBias)
{
    bias = juce::jlimit(-1.0f, 1.0f, newBias);
}

void Q5Q6GainStage::setAsymmetry(float newAsymmetry)
{
    asymmetry = juce::jlimit(0.0f, 0.95f, newAsymmetry);
}

void Q5Q6GainStage::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 12.0f, newOutputTrimDb);
}

void Q5Q6GainStage::setPreHighPassHz(float newFrequencyHz)
{
    preHighPassHz = juce::jlimit(10.0f, 1000.0f, newFrequencyHz);
    updateFilters();
}

void Q5Q6GainStage::setPostLowPassHz(float newFrequencyHz)
{
    postLowPassHz = juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.45), newFrequencyHz);

    updateFilters();
}

void Q5Q6GainStage::updateFilters()
{
    const auto safePostLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.45), postLowPassHz);

    *preHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, preHighPassHz);

    *postLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safePostLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}
