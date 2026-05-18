#include "ModernLeadPreamp.h"

#include <cmath>

ModernLeadPreamp::ModernLeadPreamp()
    : oversampler(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
}

void ModernLeadPreamp::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    baseSampleRate = sampleRate;
    oversampledSampleRate = sampleRate * oversampler.getOversamplingFactor();

    oversampler.initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler.reset();

    const auto channels = static_cast<juce::uint32>(juce::jmax(1, numChannels));

    juce::dsp::ProcessSpec baseSpec;
    baseSpec.sampleRate = baseSampleRate;
    baseSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    baseSpec.numChannels = channels;

    inputHighPass.prepare(baseSpec);
    finalLowPass.prepare(baseSpec);

    modernLowMidScoop.prepare(baseSpec);
    modernUpperMidBite.prepare(baseSpec);
    modernTopSoftening.prepare(baseSpec);

    bassShelf.prepare(baseSpec);
    midPeak.prepare(baseSpec);
    trebleShelf.prepare(baseSpec);

    juce::dsp::ProcessSpec osSpec;
    osSpec.sampleRate = oversampledSampleRate;
    osSpec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock * oversampler.getOversamplingFactor());
    osSpec.numChannels = channels;

    firstLeadStageLowPass.prepare(osSpec);
    coupling1HighPass.prepare(osSpec);

    preClipLowMidScoop.prepare(osSpec);
    preClipUpperMidPush.prepare(osSpec);

    mainLeadStageLowPass.prepare(osSpec);
    postDiodeLowPass.prepare(osSpec);
    coupling2HighPass.prepare(osSpec);

    reset();
}

void ModernLeadPreamp::reset()
{
    oversampler.reset();

    inputHighPass.reset();
    finalLowPass.reset();

    preClipLowMidScoop.reset();
    preClipUpperMidPush.reset();

    firstLeadStageLowPass.reset();
    coupling1HighPass.reset();

    mainLeadStageLowPass.reset();
    postDiodeLowPass.reset();
    coupling2HighPass.reset();

    modernLowMidScoop.reset();
    modernUpperMidBite.reset();
    modernTopSoftening.reset();

    bassShelf.reset();
    midPeak.reset();
    trebleShelf.reset();
}

void ModernLeadPreamp::processBlock(
    juce::AudioBuffer<float> &buffer,
    float drive01,
    float bassDb,
    float midDb,
    float trebleDb,
    float postGainLinear)
{
    drive01 = juce::jlimit(0.0f, 1.0f, drive01);

    updateBaseRateFilters(bassDb, midDb, trebleDb);
    updateOversampledFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    // Input coupling/tightening before the lead distortion path.
    // Do not add another dirty-path HPF in AmpPreamp before this.
    processFilter(inputHighPass, block);

    // Oversampled nonlinear lead path.
    auto osBlock = oversampler.processSamplesUp(block);
    processNonlinearBlock(osBlock, drive01);
    oversampler.processSamplesDown(block);

    // Fixed Modern voicing before the user tone controls.
    processFilter(modernLowMidScoop, block);
    processFilter(modernUpperMidBite, block);
    processFilter(modernTopSoftening, block);

    // Practical tone-stack approximation.
    processFilter(bassShelf, block);
    processFilter(midPeak, block);
    processFilter(trebleShelf, block);

    // Safety anti-fizz roll-off before IR/power-amp stage.
    processFilter(finalLowPass, block);

    buffer.applyGain(calibration.outputTrim * postGainLinear);
}

void ModernLeadPreamp::processNonlinearBlock(juce::dsp::AudioBlock<float> &block, float drive01)
{
    // Modern capture direction:
    // - keep body into the clamp
    // - let the clamp be firm and asymmetric
    // - use recovery/makeup after clipping for density
    // - scoop after the damage, not mostly before it
    const auto d1 = shapeDrive(drive01, 1.20f);
    const auto d2 = shapeDrive(drive01, 0.85f);
    const auto d3 = shapeDrive(drive01, 1.75f);

    const auto firstLeadStageGain = juce::jmap(d1, 1.0f, calibration.firstLeadStageMaxGain);
    const auto mainLeadStageGain = juce::jmap(d2, 0.7f, calibration.mainLeadStageMaxGain);
    const auto postLimiterRecoveryGain = juce::jmap(d3, 1.0f, calibration.postLimiterRecoveryGain);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i)
            samples[i] *= calibration.inputTrim;
    }

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            x = transistorPair(
                x,
                firstLeadStageGain,
                calibration.firstLeadStagePositiveShape,
                calibration.firstLeadStageNegativeShape,
                calibration.firstLeadStageOutputTrim);

            samples[i] = x;
        }
    }

    processFilter(firstLeadStageLowPass, block);
    processFilter(coupling1HighPass, block);

    // This is deliberately subtle. The old values removed too much chug/body before clipping.
    processFilter(preClipLowMidScoop, block);
    processFilter(preClipUpperMidPush, block);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            // Main Modern drive stage. This is a firmer diode-like wall, not tanh saturation.
            x = modernClip(
                x,
                mainLeadStageGain,
                calibration.diodePositiveThreshold,
                calibration.diodeNegativeThreshold,
                calibration.diodePositiveOverSlope,
                calibration.diodeNegativeOverSlope,
                calibration.diodeAsymmetry);

            samples[i] = x * calibration.diodeOutputTrim;
        }
    }

    // Gain knob should add saturation without mathematically undoing the whole clipped result.
    const auto driveCompensation = juce::jmap(d2, 1.0f, 0.55f);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i)
            samples[i] *= driveCompensation;
    }

    processFilter(mainLeadStageLowPass, block);
    processFilter(postDiodeLowPass, block);
    processFilter(coupling2HighPass, block);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            x = transistorPair(
                x,
                postLimiterRecoveryGain,
                calibration.postLimiterRecoveryPositiveShape,
                calibration.postLimiterRecoveryNegativeShape,
                calibration.postLimiterRecoveryOutputTrim);

            // Q11-ish buffer. Extremely subtle for Modern; mostly clean.
            const auto buffered =
                std::tanh(x * calibration.q11BufferDrive) / std::tanh(calibration.q11BufferDrive);

            x = mix(x, buffered, calibration.q11BufferMix);

            samples[i] = x;
        }
    }
}

void ModernLeadPreamp::updateBaseRateFilters(float bassDb, float midDb, float trebleDb)
{
    // The UI may expose these as +/-12 dB style controls, but the real Bandit
    // sweep is more constrained and amp-like. Modern mid cut is allowed to be
    // slightly more savage than Vintage.
    const auto modernBassDb = mapControlDb(bassDb, -8.0f, 5.0f);
    const auto modernMidDb = mapControlDb(midDb, -9.0f, 3.5f);
    const auto modernTrebleDb = mapControlDb(trebleDb, -8.0f, 6.0f);

    *inputHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        baseSampleRate, calibration.inputHighPassHz);

    *finalLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        baseSampleRate, calibration.finalLowPassHz);

    *modernLowMidScoop.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.modernLowMidScoopHz,
        calibration.modernLowMidScoopQ,
        dbToGain(calibration.modernLowMidScoopDb));

    *modernUpperMidBite.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.modernUpperMidBiteHz,
        calibration.modernUpperMidBiteQ,
        dbToGain(calibration.modernUpperMidBiteDb));

    *modernTopSoftening.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        baseSampleRate, calibration.modernTopSofteningHz);

    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        baseSampleRate, calibration.bassShelfHz, calibration.bassShelfQ, dbToGain(modernBassDb));

    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate, calibration.midPeakHz, calibration.midPeakQ, dbToGain(modernMidDb));

    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        baseSampleRate,
        calibration.trebleShelfHz,
        calibration.trebleShelfQ,
        dbToGain(modernTrebleDb));
}

void ModernLeadPreamp::updateOversampledFilters()
{
    *firstLeadStageLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.firstLeadStageLowPassHz);

    *coupling1HighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.coupling1HighPassHz);

    *mainLeadStageLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.mainLeadStageLowPassHz);

    *postDiodeLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.postDiodeLowPassHz);

    *coupling2HighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.coupling2HighPassHz);

    *preClipLowMidScoop.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.preClipLowMidScoopHz,
        calibration.preClipLowMidScoopQ,
        dbToGain(calibration.preClipLowMidScoopDb));

    *preClipUpperMidPush.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.preClipUpperMidPushHz,
        calibration.preClipUpperMidPushQ,
        dbToGain(calibration.preClipUpperMidPushDb));
}

float ModernLeadPreamp::dbToGain(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float ModernLeadPreamp::shapeDrive(float drive01, float curve)
{
    drive01 = juce::jlimit(0.0f, 1.0f, drive01);
    return std::pow(drive01, curve);
}

float ModernLeadPreamp::mapControlDb(float valueDb, float minDb, float maxDb)
{
    const auto normalised =
        juce::jmap(juce::jlimit(-12.0f, 12.0f, valueDb), -12.0f, 12.0f, 0.0f, 1.0f);

    return juce::jmap(normalised, minDb, maxDb);
}

float ModernLeadPreamp::transistorPair(
    float x, float gain, float positiveShape, float negativeShape, float outputTrim)
{
    x *= gain;

    const auto posDrive = juce::jmax(0.05f, positiveShape);
    const auto negDrive = juce::jmax(0.05f, negativeShape);

    if (x >= 0.0f)
        x = x / (1.0f + posDrive * std::abs(x));
    else
        x = x / (1.0f + negDrive * std::abs(x));

    return x * outputTrim;
}

float ModernLeadPreamp::modernClip(
    float x,
    float drive,
    float positiveThreshold,
    float negativeThreshold,
    float positiveOverSlope,
    float negativeOverSlope,
    float asymmetry)
{
    x = (x * drive) + asymmetry;

    positiveThreshold = juce::jmax(0.05f, positiveThreshold);
    negativeThreshold = juce::jmax(0.05f, negativeThreshold);
    positiveOverSlope = juce::jlimit(0.0f, 0.25f, positiveOverSlope);
    negativeOverSlope = juce::jlimit(0.0f, 0.25f, negativeOverSlope);

    if (x > positiveThreshold)
        x = positiveThreshold + (x - positiveThreshold) * positiveOverSlope;
    else if (x < -negativeThreshold)
        x = -negativeThreshold + (x + negativeThreshold) * negativeOverSlope;

    return x - asymmetry;
}

float ModernLeadPreamp::mix(float dry, float wet, float wetMix)
{
    wetMix = juce::jlimit(0.0f, 1.0f, wetMix);
    return dry + wetMix * (wet - dry);
}

void ModernLeadPreamp::processFilter(Filter &filter, juce::dsp::AudioBlock<float> &block)
{
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
}
