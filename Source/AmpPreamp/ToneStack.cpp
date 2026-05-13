
#include "ToneStack.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_dsp/juce_dsp.h"
#include <cmath>

void ToneStack::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    bassShelfFilter.prepare(spec);
    lowerMidPeakFilter.prepare(spec);
    upperMidPeakFilter.prepare(spec);
    trebleShelfFilter.prepare(spec);
}

void ToneStack::processBlock(
    juce::AudioBuffer<float> &buffer,
    float bassDb,
    float lowerMidDb,
    float upperMidDb,
    float trebleDb)
{
    *bassShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        currentSampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(bassDb));

    *lowerMidPeakFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, 400.0f, 0.55f, juce::Decibels::decibelsToGain(lowerMidDb));

    *upperMidPeakFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, 1500.0f, 0.45f, juce::Decibels::decibelsToGain(upperMidDb));

    *trebleShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, 4000.0f, 0.707f, juce::Decibels::decibelsToGain(trebleDb));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    bassShelfFilter.process(context);
    lowerMidPeakFilter.process(context);
    upperMidPeakFilter.process(context);
    trebleShelfFilter.process(context);
}
