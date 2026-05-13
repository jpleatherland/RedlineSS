#pragma once

#include <juce_dsp/juce_dsp.h>

class AmpPowerAmp {
  public:
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);

    void processBlock(juce::AudioBuffer<float> &buffer, float resonanceDb);

  private:
    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            resonance;

    double currentSampleRate = 44100.0;
};
