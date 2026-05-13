#pragma once

#include <juce_dsp/juce_dsp.h>

class ToneStack {
  public:
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock(
        juce::AudioBuffer<float> &buffer,
        float bassDb,
        float lowerMidDb,
        float upperMidDb,
        float trebleDb);

  private:
    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            bassShelfFilter;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            lowerMidPeakFilter;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            upperMidPeakFilter;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            trebleShelfFilter;

    double currentSampleRate = 44100.0;
};
