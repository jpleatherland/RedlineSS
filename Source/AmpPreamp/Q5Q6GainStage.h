#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class Q5Q6GainStage {
  public:
    Q5Q6GainStage() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setDriveDb(float newDriveDb);
    void setBias(float newBias);
    void setAsymmetry(float newAsymmetry);
    void setOutputTrimDb(float newOutputTrimDb);
    void setPreHighPassHz(float newFrequencyHz);
    void setPostLowPassHz(float newFrequencyHz);

  private:
    void updateFilters();

    static float shapeSample(float x, float bias, float asymmetry);

    double sampleRate = 44100.0;
    int channelCount = 2;

    // This is the first real lead-channel voltage-gain approximation.
    float driveDb = 28.0f;

    // Bias/asymmetry are deliberately audible here.
    float bias = 0.12f;
    float asymmetry = 0.35f;

    // Block2 in LTspice may be very hot; keep plugin levels sane.
    float outputTrimDb = -12.0f;

    float preHighPassHz = 70.0f;
    float postLowPassHz = 12000.0f;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            preHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            postLowPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            outputDcBlocker;
};
