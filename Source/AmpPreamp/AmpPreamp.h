#pragma once

#include <juce_dsp/juce_dsp.h>

#include "GainStage1.h"
#include "GainStage2.h"
#include "GainStage3.h"
#include "ToneStack.h"

class AmpPreamp {
  public:
    AmpPreamp();
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);

    void processBlock(
        juce::AudioBuffer<float> &buffer,
        float inputHighPassHz,
        bool dirty,
        float gain1,
        float bias1,
        float interstageHighPassHz,
        float interstageHighPassHz2,
        float gain2,
        float gain3,
        float bassDb,
        float lowerMidDb,
        float upperMidDb,
        float trebleDb,
        float fizzLowPassHz,
        float masterLevel);

  private:
    GainStage1 gainStage1;
    GainStage2 gainStage2;
    GainStage3 gainStage3;
    ToneStack toneStack;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            inputHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            interstageHighPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            interstageLowPass;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            interstageHighPass2;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            interstageLowPass2;

    juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
            fizzLowPass;

    juce::dsp::Oversampling<float> oversampler;

    double currentSampleRate = 44100.0;
};
