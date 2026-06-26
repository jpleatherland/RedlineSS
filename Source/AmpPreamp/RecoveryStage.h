#pragma once

#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

class RecoveryStage {
  public:
    RecoveryStage() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void setMakeupGainDb(float newMakeupGainDb);
    void setDriveDb(float newDriveDb);
    void setBias(float newBias);
    void setAsymmetry(float newAsymmetry);
    void setKnee(float newKnee);
    void setOutputTrimDb(float newOutputTrimDb);
    void setPreHighPassHz(float newFrequencyHz);
    void setPostLowPassHz(float newFrequencyHz);
    void setBodyDb(float newBodyDb);
    void setBodyFrequencyHz(float newFrequencyHz);
    void setDefizzDb(float newDefizzDb);
    void setDefizzFrequencyHz(float newDefizzFrequencyHz);

  private:
    void updateFilters();

    static float limitWithKnee(float x, float lowerLimit, float upperLimit, float knee);

    static float shapeSample(float x, float shapeBias, float shapeAsymmetry, float knee);

    double sampleRate = 44100.0;
    int channelCount = 2;

    float makeupGainDb = 12.0f;

    // Recovery after the clamp. Enough gain to restore level, not a new main dirt source.
    float driveDb = 36.0f;

    // Mild transistor bias/asymmetry.
    float bias = 0.18f;
    float asymmetry = 0.60f;

    // Larger than clamp knee: this stage should recover, shape and drive.
    float knee = 0.02f;

    float outputTrimDb = -18.0f;

    // C25 into R39 is ~3.4 Hz, so this is mostly DC/sub cleanup.
    float preHighPassHz = 20.0f;

    // C68 470p across 1Meg is a tiny treble shunt, but effective loading is more complex.
    // Keep it open initially.
    float postLowPassHz = 22000.0f;

    float bodyDb = 2.0f;
    float bodyFrequencyHz = 180.0f;

    float defizzDb = -2.0f;
    float defizzFrequencyHz = 6500.0f;

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
            bodyShelf;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            defizzShelf;
};
