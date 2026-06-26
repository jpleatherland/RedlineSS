#include "InputConditioning.h"

void InputConditioning::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    channelCount = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(channelCount);

    inputDcBlocker.prepare(spec);
    rfLowPass.prepare(spec);
    outputDcBlocker.prepare(spec);

    updateFilters();
    reset();
}

void InputConditioning::reset()
{
    inputDcBlocker.reset();
    rfLowPass.reset();
    outputDcBlocker.reset();
}

void InputConditioning::process(juce::AudioBuffer<float> &buffer)
{
    if (buffer.getNumSamples() == 0)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    inputDcBlocker.process(context);

    const float inputStageGain = juce::Decibels::decibelsToGain(inputStageGainDb);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        auto *samples = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float x = samples[sample];

            // Approximate the shared transistor input stage level lift.
            x *= inputStageGain;

            // Mild asymmetric transistor-ish shaping.
            // This should remain subtle; the lead channel does the real damage later.
            const float biasedInput = (x + inputStageBias) * inputStageDrive;
            const float shaped = std::tanh(biasedInput);

            // Remove the DC shift caused by biasing the waveshaper.
            const float biasCorrection = std::tanh(inputStageBias * inputStageDrive);

            x = (shaped - biasCorrection) / inputStageDrive;

            // Small post-stage trim to stop this block from becoming the main clipper.
            x *= 0.95f;

            samples[sample] = x;
        }
    }

    rfLowPass.process(context);
    outputDcBlocker.process(context);
}

void InputConditioning::setInputStageGainDb(float gainDb)
{
    inputStageGainDb = gainDb;
}

void InputConditioning::setInputStageDrive(float drive)
{
    inputStageDrive = juce::jmax(0.01f, drive);
}

void InputConditioning::setInputStageBias(float bias)
{
    inputStageBias = bias;
}

void InputConditioning::setRfLowPassHz(float frequencyHz)
{
    rfLowPassHz = juce::jlimit(1000.0f, 22000.0f, frequencyHz);
    updateFilters();
}

void InputConditioning::updateFilters()
{
    const auto safeLowPassHz =
        juce::jlimit(1000.0f, static_cast<float>(sampleRate * 0.45), rfLowPassHz);

    *inputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

    *rfLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, safeLowPassHz);

    *outputDcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}
