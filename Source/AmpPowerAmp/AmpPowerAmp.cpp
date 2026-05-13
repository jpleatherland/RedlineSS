#include "AmpPowerAmp.h"

void AmpPowerAmp::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    resonance.prepare(spec);
}

void AmpPowerAmp::processBlock(juce::AudioBuffer<float> &buffer, float resonanceDb)
{
    *resonance.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, 100.0f, 1.0f, juce::Decibels::decibelsToGain(resonanceDb));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    resonance.process(context);
}
