#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class LeadPreGain {
  public:
    LeadPreGain() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setPreGain(float newPreGain);
    void setBrightAmount(float newBrightAmount);
    void setOutputTrimDb(float newOutputTrimDb);

  private:
    void updateFilters();

    double sampleRate = 44100.0;
    int channelCount = 2;

    // 0.0 = minimum pre-gain, 1.0 = maximum pre-gain.
    float preGain = 0.75f;

    // Overall strength of the gain-pot bright bypass approximation.
    float brightAmount = 0.45f;

    // Use this to calibrate the level feeding Q5/Q6 later.
    float outputTrimDb = -3.0f;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            couplingHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            brightHighPass;

    juce::AudioBuffer<float> brightBuffer;
};
