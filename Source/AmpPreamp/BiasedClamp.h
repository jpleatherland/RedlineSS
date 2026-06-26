#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class BiasedClamp {
  public:
    BiasedClamp() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setInputTrimDb(float newInputTrimDb);
    void setBias(float newBias);
    void setPositiveLimit(float newPositiveLimit);
    void setNegativeLimit(float newNegativeLimit);
    void setKnee(float newKnee);
    void setOutputTrimDb(float newOutputTrimDb);
    void setPreHighPassHz(float newFrequencyHz);
    void setPostLowPassHz(float newFrequencyHz);

  private:
    void updateFilters();

    static float limitWithKnee(float x, float lowerLimit, float upperLimit, float knee);

    static float
    shapeSample(float x, float clampBias, float positiveLimit, float negativeLimit, float knee);

    double sampleRate = 44100.0;
    int channelCount = 2;

    float inputTrimDb = 0.0f;

    // Bias pushes the signal into an asymmetric clamp window before being removed.
    float bias = 0.20f;

    // Positive and negative clamp limits are intentionally independent.
    // The negative limit is stored as a positive magnitude.
    float positiveLimit = 0.85f;
    float negativeLimit = 0.45f;

    // Small knee = hard limiter-ish. Larger knee = softer diode-ish bend.
    float knee = 0.04f;

    float outputTrimDb = -3.0f;

    float preHighPassHz = 20.0f;
    float postLowPassHz = 22000.0f;

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
