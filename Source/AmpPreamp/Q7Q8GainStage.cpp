#include "Q7Q8GainStage.h"

void Q7Q8GainStage::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
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
    presenceShelf.prepare(spec);

    updateFilters();
    reset();
}

void Q7Q8GainStage::reset()
{
    preHighPass.reset();
    postLowPass.reset();
    outputDcBlocker.reset();
    presenceShelf.reset();
}

void Q7Q8GainStage::process(juce::AudioBuffer<float> &buffer)
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

            x = shapeSample(x, bias, asymmetry, compression);

            // Another common-emitter-ish collector output, so invert again.
            // This helps if comparing against LTspice block polarity.
            x = -x;

            x *= outputTrim;

            samples[sample] = x;
        }
    }

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        presenceShelf.process(context);
        postLowPass.process(context);
        outputDcBlocker.process(context);
    }
}

static float limitWithKnee(float x, float lowerLimit, float upperLimit, float kneeAmount)
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

float Q7Q8GainStage::shapeSample(
    float x, float shapeBias, float shapeAsymmetry, float compressionAmount)
{
    juce::ignoreUnused(compressionAmount);

    const float biased = x + shapeBias;

    constexpr float evenAmount = 0.18f;
    constexpr float oddAmount = 0.055f;

    auto curve = [](float v) { return v + evenAmount * v * v - oddAmount * v * v * v; };

    float curved = curve(biased);

    const float zeroPoint = curve(shapeBias);
    curved -= zeroPoint;

    const float upperLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.95f, 1.35f);

    const float lowerLimit = -juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.95f, 0.45f);

    constexpr float knee = 0.12f;

    return limitWithKnee(curved, lowerLimit, upperLimit, knee);
}

// float Q7Q8GainStage::shapeSample(
//     float x, float shapeBias, float shapeAsymmetry, float compressionAmount)
// {
//     const float biased = x + shapeBias;
//
//     const float positiveLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.95f, 1.25f);
//
//     const float negativeLimit = juce::jmap(shapeAsymmetry, 0.0f, 1.0f, 0.85f, 0.35f);
//
//     auto hardishAsymmetricClip = [positiveLimit, negativeLimit](float v) {
//         if (v > positiveLimit)
//             return positiveLimit;
//
//         if (v < -negativeLimit)
//             return -negativeLimit;
//
//         return v;
//     };
//
//     float clipped = hardishAsymmetricClip(biased);
//
//     const float zeroPoint = hardishAsymmetricClip(shapeBias);
//     clipped -= zeroPoint;
//
//     // Add a controlled edge-enhancing nonlinearity.
//     // This brings fizz/harmonics back without turning the whole stage into a woolly tanh
//     blanket. const float edgeDrive = 1.0f + compressionAmount * 4.0f; const float edged = clipped
//     + 0.18f * std::tanh(clipped * edgeDrive * 3.0f);
//
//     return edged;
// }

void Q7Q8GainStage::setDriveDb(float newDriveDb)
{
    driveDb = juce::jlimit(0.0f, 48.0f, newDriveDb);
}

void Q7Q8GainStage::setBias(float newBias)
{
    bias = juce::jlimit(-1.0f, 1.0f, newBias);
}

void Q7Q8GainStage::setAsymmetry(float newAsymmetry)
{
    asymmetry = juce::jlimit(0.0f, 0.95f, newAsymmetry);
}

void Q7Q8GainStage::setCompression(float newCompression)
{
    compression = juce::jlimit(0.0f, 1.0f, newCompression);
}

void Q7Q8GainStage::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 12.0f, newOutputTrimDb);
}

void Q7Q8GainStage::setPreHighPassHz(float newFrequencyHz)
{
    preHighPassHz = juce::jlimit(10.0f, 1000.0f, newFrequencyHz);
    updateFilters();
}

void Q7Q8GainStage::setPresenceLiftDb(float newGainDb)
{
    presenceLiftDb = juce::jlimit(-6.0f, 12.0f, newGainDb);
    updateFilters();
}

void Q7Q8GainStage::setPostLowPassHz(float newFrequencyHz)
{
    postLowPassHz = juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.45), newFrequencyHz);

    updateFilters();
}

void Q7Q8GainStage::updateFilters()
{
    const auto safePostLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.45), postLowPassHz);

    *presenceShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 2000.0f, 0.7f, juce::Decibels::decibelsToGain(presenceLiftDb));

    *preHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, preHighPassHz);

    *postLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safePostLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}
