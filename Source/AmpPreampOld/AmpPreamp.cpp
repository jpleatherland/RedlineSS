#include "AmpPreamp.h"

AmpPreamp::AmpPreamp()
    : oversampler(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
}

void AmpPreamp::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;

    oversampler.initProcessing(static_cast<juce::uint32>(samplesPerBlock));
    oversampler.reset();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    inputHighPass.prepare(spec);
    interstageHighPass.prepare(spec);
    interstageLowPass.prepare(spec);
    interstageHighPass2.prepare(spec);
    interstageLowPass2.prepare(spec);
    fizzLowPass.prepare(spec);

    gainStage1.prepare(sampleRate);
    gainStage2.prepare(sampleRate);
    gainStage3.prepare(sampleRate);
    toneStack.prepare(sampleRate, samplesPerBlock, numChannels);
    vintageLeadPreamp.prepare(sampleRate, samplesPerBlock, numChannels);
    modernLeadPreamp.prepare(sampleRate, samplesPerBlock, numChannels);
}

void AmpPreamp::processBlock(
    juce::AudioBuffer<float> &buffer,
    float inputHighPassHz,
    bool highGainInput,
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
    float masterLevel)
{
    // mimic the high gain input add 6db
    const float inputJackGain = highGainInput ? 6.0f : 0.0f;

    buffer.applyGain(juce::Decibels::decibelsToGain(inputJackGain));

    // Modern lead has its own input coupling, oversampling, nonlinear path,
    // voicing filters and final low-pass. Do not pre-filter it here, otherwise
    // the low/lower-mid body is removed before the clamp has anything to bite on.
    juce::ignoreUnused(
        inputHighPassHz,
        bias1,
        interstageHighPassHz,
        interstageHighPassHz2,
        gain2,
        gain3,
        upperMidDb,
        fizzLowPassHz);

    modernLeadPreamp.processBlock(buffer, gain1 / 10.0f, bassDb, lowerMidDb, trebleDb, masterLevel);
    return;

    *inputHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, inputHighPassHz);

    *interstageHighPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, interstageHighPassHz);

    *interstageHighPass2.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        currentSampleRate, interstageHighPassHz2);

    *fizzLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, fizzLowPassHz);

    *interstageLowPass.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 7000.0f);

    *interstageLowPass2.state =
        *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, 6500.0f);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Legacy/non-modern path. Left intact, apart from moving the Modern return above it.
    inputHighPass.process(context);

    auto oversampledBlock = oversampler.processSamplesUp(block);
    juce::dsp::ProcessContextReplacing<float> oversampledContext(oversampledBlock);

    // If you want the Vintage lead macro-model here instead of the old generic chain,
    // replace this legacy block with vintageLeadPreamp.processBlock(...).

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel) {
        auto *data = oversampledBlock.getChannelPointer(channel);

        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            data[i] = gainStage1.processSample(data[i], gain1, bias1);
    }

    interstageHighPass.process(oversampledContext);
    interstageLowPass.process(oversampledContext);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel) {
        auto *data = oversampledBlock.getChannelPointer(channel);

        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            data[i] = gainStage2.processSample(data[i], gain2);
    }

    interstageHighPass2.process(oversampledContext);
    interstageLowPass2.process(oversampledContext);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel) {
        auto *data = oversampledBlock.getChannelPointer(channel);

        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            data[i] = gainStage3.processSample(data[i], gain3);
    }

    oversampler.processSamplesDown(block);

    // Tone stack is still disabled here, matching your current implementation.
    // toneStack.processBlock(buffer, bassDb, lowerMidDb, upperMidDb, trebleDb);

    fizzLowPass.process(context);

    buffer.applyGain(masterLevel);
}
