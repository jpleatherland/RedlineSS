#include "LeadPreGain.h"

void LeadPreGain::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    channelCount = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(channelCount);

    couplingHighPass.prepare(spec);
    brightHighPass.prepare(spec);

    brightBuffer.setSize(channelCount, samplesPerBlock);

    updateFilters();
    reset();
}

void LeadPreGain::reset()
{
    couplingHighPass.reset();
    brightHighPass.reset();
    brightBuffer.clear();
}

void LeadPreGain::process(juce::AudioBuffer<float> &buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    if (numSamples == 0)
        return;

    if (brightBuffer.getNumChannels() < numChannels || brightBuffer.getNumSamples() < numSamples) {
        brightBuffer.setSize(numChannels, numSamples, false, false, true);
    }

    // Approximate coupling cap / lead-entry low-end tightening.
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        couplingHighPass.process(context);
    }

    brightBuffer.makeCopyOf(buffer, true);

    // Bright bypass path: filtered copy blended back in.
    {
        juce::dsp::AudioBlock<float> brightBlock(brightBuffer);
        juce::dsp::ProcessContextReplacing<float> brightContext(brightBlock);
        brightHighPass.process(brightContext);
    }

    // Gain pot response.
    //
    // This is intentionally non-linear. Guitar amp gain pots rarely feel linear,
    // and the useful action tends to ramp harder in the upper half.
    const float gain01 = juce::jlimit(0.0f, 1.0f, preGain);

    const float baseGainDb = juce::jmap(gain01, 0.0f, 1.0f, -18.0f, 24.0f);

    // Extra upper-half shove. This helps the later Q5/Q6 stage get hit properly
    // without making the bottom half of the control useless.
    const float upperHalf = juce::jlimit(0.0f, 1.0f, (gain01 - 0.5f) * 2.0f);
    const float upperHalfGainDb = upperHalf * upperHalf * 10.0f;

    const float mainGain = juce::Decibels::decibelsToGain(baseGainDb + upperHalfGainDb);

    // Bright cap is strongest when the pot is not fully maxed.
    // At very low gain, not much signal gets through anyway.
    // Around the middle/upper-middle it has the most bite.
    const float brightWindow = std::sin(gain01 * juce::MathConstants<float>::pi);

    const float brightGain = brightAmount * brightWindow * 0.75f;

    const float outputTrim = juce::Decibels::decibelsToGain(outputTrimDb);

    for (int channel = 0; channel < numChannels; ++channel) {
        auto *dry = buffer.getWritePointer(channel);
        const auto *bright = brightBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float x = dry[sample];

            // Main pot path plus bright-bypass contribution.
            x = (x + bright[sample] * brightGain) * mainGain;

            // Gentle safety softening only.
            // This is not intended to be the lead clipping stage.
            x = std::tanh(x * 0.85f) / 0.85f;

            x *= outputTrim;

            dry[sample] = x;
        }
    }
}

void LeadPreGain::setPreGain(float newPreGain)
{
    preGain = juce::jlimit(0.0f, 1.0f, newPreGain);
}

void LeadPreGain::setBrightAmount(float newBrightAmount)
{
    brightAmount = juce::jlimit(0.0f, 2.0f, newBrightAmount);
}

void LeadPreGain::setOutputTrimDb(float newOutputTrimDb)
{
    outputTrimDb = juce::jlimit(-48.0f, 24.0f, newOutputTrimDb);
}

void LeadPreGain::updateFilters()
{
    // Rough coupling / lead path tightening.
    //
    // Keep this fairly low for now. The big "Modern tightness" should come
    // later from the lead voicing/tone-stack area, not all here.
    *couplingHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 300.0f);

    // Bright bypass approximation around the pre-gain pot.
    *brightHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 1200.0f);
}
