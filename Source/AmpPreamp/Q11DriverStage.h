#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class Q11DriverStage {
  public:
    Q11DriverStage() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setInputTrimDb(float newInputTrimDb);
    void setDriveDb(float newDriveDb);
    void setBias(float newBias);
    void setAsymmetry(float newAsymmetry);
    void setKnee(float newKnee);
    void setOutputTrimDb(float newOutputTrimDb);

    void setPreHighPassHz(float newFrequencyHz);
    void setBodyDb(float newBodyDb);
    void setBodyFrequencyHz(float newFrequencyHz);
    void setMidShapeDb(float newMidShapeDb);
    void setPresenceDb(float newPresenceDb);
    void setPostLowPassHz(float newFrequencyHz);

  private:
    void updateFilters();

    static float limitWithKnee(float x, float lowerLimit, float upperLimit, float knee);

    static float shapeSample(float x, float shapeBias, float shapeAsymmetry, float knee);

    double sampleRate = 44100.0;
    int channelCount = 2;

    float inputTrimDb = 0.0f;

    // Q11 should drive/shape, not be another full drive stage initially.
    float driveDb = 8.0f;
    float bias = -0.08f; // PNP-ish opposite bias direction from earlier NPN-ish stages.
    float asymmetry = 0.35f;
    float knee = 0.10f;

    float outputTrimDb = -3.0f;

    float preHighPassHz = 30.0f;

    // Block6 shaping guesses. These are meant for matching against the LTspice Block6 curve.
    float bodyDb = 0.0f;
    float bodyFrequencyHz = 160.0f;

    float midShapeDb = 0.0f;
    float presenceDb = 0.0f;

    float postLowPassHz = 18000.0f;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            preHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            bodyShelf;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            midShape;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            presenceShelf;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            postLowPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            outputDcBlocker;
};
