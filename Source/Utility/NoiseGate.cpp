#include "NoiseGate.h"

void NoiseGate::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    gate.prepare(spec);

    gate.setRatio(12.0f);
    gate.setAttack(1.0f);
    gate.setRelease(80.0f);
}

void NoiseGate::processBlock(juce::AudioBuffer<float> &buffer, float thresholdDb)
{
    gate.setThreshold(thresholdDb);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    gate.process(context);
}
