#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class InputConditioning {
  public:
    InputConditioning() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setInputStageGainDb(float gainDb);
    void setInputStageDrive(float drive);
    void setInputStageBias(float bias);
    void setRfLowPassHz(float frequencyHz);

  private:
    void updateFilters();

    double sampleRate = 44100.0;
    int channelCount = 2;

    float inputStageGainDb = 12.0f;
    float inputStageDrive = 1.25f;
    float inputStageBias = 0.08f;
    float rfLowPassHz = 18000.0f;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            inputDcBlocker;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            rfLowPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            outputDcBlocker;
};
