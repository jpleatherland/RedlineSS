#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class Q7Q8GainStage {
  public:
    Q7Q8GainStage() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setDriveDb(float newDriveDb);
    void setBias(float newBias);
    void setAsymmetry(float newAsymmetry);
    void setCompression(float newCompression);
    void setOutputTrimDb(float newOutputTrimDb);
    void setPreHighPassHz(float newFrequencyHz);
    void setPostLowPassHz(float newFrequencyHz);
    void setPresenceLiftDb(float newGainDb);

  private:
    void updateFilters();

    static float
    shapeSample(float x, float shapeBias, float shapeAsymmetry, float compressionAmount);

    double sampleRate = 44100.0;
    int channelCount = 2;

    // Second active lead gain stage. Less "jump" than Q5/Q6, more density.
    float driveDb = 22.0f;

    float bias = 0.10f;
    float asymmetry = 0.40f;

    // Extra firming after Q5/Q6 so the clamp receives a dense signal.
    float compression = 0.35f;

    float outputTrimDb = -10.0f;

    float preHighPassHz = 110.0f;
    float postLowPassHz = 10500.0f;

    float presenceLiftDb = 0.0f;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            preHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            postLowPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            outputDcBlocker;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            presenceShelf;
};
