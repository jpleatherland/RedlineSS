#pragma once

#include <juce_dsp/juce_dsp.h>

class NoiseGate {
  public:
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock(juce::AudioBuffer<float> &buffer, float thresholdDb);

  private:
    juce::dsp::NoiseGate<float> gate;
};
