#include "Q11DriverStage.h"

void Q11DriverStage::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    channelCount = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(channelCount);

    preHighPass.prepare(spec);
    bodyShelf.prepare(spec);
    midShape.prepare(spec);
    presenceShelf.prepare(spec);
    postLowPass.prepare(spec);
    outputDcBlocker.prepare(spec);

    updateFilters();
    reset();
}

void Q11DriverStage::reset()
{
    preHighPass.reset();
    bodyShelf.reset();
    midShape.reset();
    presenceShelf.reset();
    postLowPass.reset();
    outputDcBlocker.reset();
}

void Q11DriverStage::process(juce::AudioBuffer<float> &buffer)
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
    const float drive = juce::Decibels::decibelsToGain(driveDb);
    const float outputTrim = juce::Decibels::decibelsToGain(outputTrimDb);

    for (int channel = 0; channel < numChannels; ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float x = samples[sample];

            x *= inputTrim;
            x *= drive;

            x = shapeSample(x, bias, asymmetry, knee);

            // Q11 is a PNP/common-emitter-ish driver. Keep inversion for now.
            // Remove this if polarity comparison against Block6 says otherwise.
            x = -x;

            x *= outputTrim;

            samples[sample] = x;
        }
    }

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        bodyShelf.process(context);
        midShape.process(context);
        presenceShelf.process(context);
        postLowPass.process(context);
        outputDcBlocker.process(context);
    }
}

float Q11DriverStage::limitWithKnee(float x, float lowerLimit, float upperLimit, float kneeAmount)
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

float Q11DriverStage::shapeSample(float x, float shapeBias, float shapeAsymmetry, float kneeAmount)
{
    const float biased = x + shapeBias;

    // Wider and gentler than the hard clamp/recovery stages.
    const float upperLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.95f, 1.35f);

    const float lowerLimit = -juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.95f, 0.50f);

    const float limited = limitWithKnee(biased, lowerLimit, upperLimit, kneeAmount);

    const float zeroPoint = limitWithKnee(shapeBias, lowerLimit, upperLimit, kneeAmount);

    return limited - zeroPoint;
}

void Q11DriverStage::setInputTrimDb(float newInputTrimDb)
{
    inputTrimDb = juce::jlimit(-36.0f, 36.0f, newInputTrimDb);
}

void Q11DriverStage::setDriveDb(float newDriveDb)
{
    driveDb = juce::jlimit(-12.0f, 48.0f, newDriveDb);
}

void Q11DriverStage::setBias(float newBias)
{
    bias = juce::jlimit(-1.0f, 1.0f, newBias);
}

void Q11DriverStage::setAsymmetry(float newAsymmetry)
{
    asymmetry = juce::jlimit(0.0f, 1.0f, newAsymmetry);
}

void Q11DriverStage::setKnee(float newKnee)
{
    knee = juce::jlimit(0.0001f, 2.0f, newKnee);
}

void Q11DriverStage::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 24.0f, newOutputTrimDb);
}

void Q11DriverStage::setPreHighPassHz(float newFrequencyHz)
{
    preHighPassHz = juce::jlimit(5.0f, 1000.0f, newFrequencyHz);
    updateFilters();
}

void Q11DriverStage::setBodyDb(float newBodyDb)
{
    bodyDb = juce::jlimit(-9.0f, 9.0f, newBodyDb);
    updateFilters();
}

void Q11DriverStage::setBodyFrequencyHz(float newFrequencyHz)
{
    bodyFrequencyHz = juce::jlimit(60.0f, 500.0f, newFrequencyHz);
    updateFilters();
}

void Q11DriverStage::setMidShapeDb(float newMidShapeDb)
{
    midShapeDb = juce::jlimit(-12.0f, 12.0f, newMidShapeDb);
    updateFilters();
}

void Q11DriverStage::setPresenceDb(float newPresenceDb)
{
    presenceDb = juce::jlimit(-12.0f, 12.0f, newPresenceDb);
    updateFilters();
}

void Q11DriverStage::setPostLowPassHz(float newFrequencyHz)
{
    postLowPassHz = juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), newFrequencyHz);

    updateFilters();
}

void Q11DriverStage::updateFilters()
{
    const auto safePostLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.49), postLowPassHz);

    *preHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, preHighPassHz);

    *bodyShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, bodyFrequencyHz, 0.7f, juce::Decibels::decibelsToGain(bodyDb));

    // Roughly represents the C32/R52 and surrounding contour. Tune by analyser.
    *midShape.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 1300.0f, 0.8f, juce::Decibels::decibelsToGain(midShapeDb));

    // C20/C33-ish retained bite / top contour.
    *presenceShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 2800.0f, 0.7f, juce::Decibels::decibelsToGain(presenceDb));

    *postLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safePostLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}
