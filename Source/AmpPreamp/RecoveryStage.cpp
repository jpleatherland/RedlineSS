#include "RecoveryStage.h"

void RecoveryStage::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
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
    bodyShelf.prepare(spec);
    defizzShelf.prepare(spec);

    updateFilters();
    reset();
}

void RecoveryStage::reset()
{
    preHighPass.reset();
    postLowPass.reset();
    outputDcBlocker.reset();
    bodyShelf.reset();
    defizzShelf.reset();
}

void RecoveryStage::process(juce::AudioBuffer<float> &buffer)
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
    const float makeupGain = juce::Decibels::decibelsToGain(makeupGainDb);
    const float outputTrim = juce::Decibels::decibelsToGain(outputTrimDb);

    for (int channel = 0; channel < numChannels; ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float x = samples[sample];

            x *= drive;

            x = shapeSample(x, bias, asymmetry, knee);

            // Q9/Q10 style transistor stage: keep the collector-ish inversion available.
            // If comparison against Block5 shows opposite polarity, remove this.
            x = -x;

            // make it enourmous
            x *= makeupGain;

            x *= outputTrim;

            samples[sample] = x;
        }
    }

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        bodyShelf.process(context);
        defizzShelf.process(context);
        postLowPass.process(context);
        outputDcBlocker.process(context);
    }
}

float RecoveryStage::limitWithKnee(float x, float lowerLimit, float upperLimit, float kneeAmount)
{
    jassert(lowerLimit < upperLimit);

    const float safeKnee = juce::jlimit(0.0001f, 2.0f, kneeAmount);

    if (x > upperLimit) {
        const float excess = x - upperLimit;
        return upperLimit + (excess / (1.0f + excess / safeKnee));
    }

    if (x < lowerLimit) {
        const float excess = lowerLimit - x;
        return lowerLimit - (excess / (1.0f + excess / safeKnee));
    }

    return x;
}

float RecoveryStage::shapeSample(float x, float shapeBias, float shapeAsymmetry, float kneeAmount)
{
    const float biased = x + shapeBias;

    // This stage is being hit after the clamp and then recovering hard.
    // It should slam into a transistor-ish asymmetric window.
    const float upperLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.90f, 1.45f);

    const float lowerLimit = -juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.90f, 0.32f);

    const float limited = limitWithKnee(biased, lowerLimit, upperLimit, kneeAmount);

    const float zeroPoint = limitWithKnee(shapeBias, lowerLimit, upperLimit, kneeAmount);

    return limited - zeroPoint;
}

void RecoveryStage::setMakeupGainDb(float newMakeupGainDb)
{
    makeupGainDb = juce::jlimit(-24.0f, 48.0f, newMakeupGainDb);
}

void RecoveryStage::setDriveDb(float newDriveDb)
{
    driveDb = juce::jlimit(-12.0f, 72.0f, newDriveDb);
}

void RecoveryStage::setBias(float newBias)
{
    bias = juce::jlimit(-1.0f, 1.0f, newBias);
}

void RecoveryStage::setAsymmetry(float newAsymmetry)
{
    asymmetry = juce::jlimit(0.0f, 1.0f, newAsymmetry);
}

void RecoveryStage::setKnee(float newKnee)
{
    knee = juce::jlimit(0.0001f, 2.0f, newKnee);
}

void RecoveryStage::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 24.0f, newOutputTrimDb);
}

void RecoveryStage::setPreHighPassHz(float newFrequencyHz)
{
    preHighPassHz = juce::jlimit(5.0f, 1000.0f, newFrequencyHz);
    updateFilters();
}

void RecoveryStage::setPostLowPassHz(float newFrequencyHz)
{
    postLowPassHz = juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), newFrequencyHz);

    updateFilters();
}

void RecoveryStage::updateFilters()
{
    const auto safePostLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), postLowPassHz);

    *preHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, preHighPassHz);

    *postLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safePostLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

    *bodyShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, bodyFrequencyHz, 0.7f, juce::Decibels::decibelsToGain(bodyDb));

    *defizzShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, defizzFrequencyHz, 0.7f, juce::Decibels::decibelsToGain(defizzDb));
}

void RecoveryStage::setBodyDb(float newBodyDb)
{
    bodyDb = juce::jlimit(-6.0f, 9.0f, newBodyDb);
    updateFilters();
}

void RecoveryStage::setBodyFrequencyHz(float newFrequencyHz)
{
    bodyFrequencyHz = juce::jlimit(60.0f, 500.0f, newFrequencyHz);
    updateFilters();
}

void RecoveryStage::setDefizzDb(float newDefizzDb)
{
    defizzDb = juce::jlimit(-12.0f, 3.0f, newDefizzDb);
    updateFilters();
}

void RecoveryStage::setDefizzFrequencyHz(float newFrequencyHz)
{
    defizzFrequencyHz = juce::jlimit(2500.0f, 12000.0f, newFrequencyHz);
    updateFilters();
}
