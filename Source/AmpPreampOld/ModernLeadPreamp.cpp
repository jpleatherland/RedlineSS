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
    preparedChannels = juce::jmax(1, numChannels);
    preparedBlockSize = juce::jmax(1, samplesPerBlock);

    oversampler.initProcessing(static_cast<size_t>(preparedBlockSize));
    oversampler.reset();

    juce::dsp::ProcessSpec baseSpec;
    baseSpec.sampleRate = baseSampleRate;
    baseSpec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize);
    baseSpec.numChannels = static_cast<juce::uint32>(preparedChannels);

    inputHighPass.prepare(baseSpec);
    finalLowPass.prepare(baseSpec);

    postLowShelf.prepare(baseSpec);
    postLowMid.prepare(baseSpec);
    postBodyControl.prepare(baseSpec);
    postMidHollow.prepare(baseSpec);
    postUpperRecovery.prepare(baseSpec);

    bassShelf.prepare(baseSpec);
    midPeak.prepare(baseSpec);
    trebleShelf.prepare(baseSpec);
    trebleTiltLowShelf.prepare(baseSpec);

    juce::dsp::ProcessSpec osSpec;
    osSpec.sampleRate = oversampledSampleRate;
    osSpec.maximumBlockSize =
        static_cast<juce::uint32>(preparedBlockSize * oversampler.getOversamplingFactor());
    osSpec.numChannels = static_cast<juce::uint32>(preparedChannels);

    preHighPass.prepare(osSpec);
    preLowShelf.prepare(osSpec);
    preLowMid.prepare(osSpec);
    preMidHollow.prepare(osSpec);
    preUpperPush.prepare(osSpec);

    interstageHighPass.prepare(osSpec);
    interstageLowShelf.prepare(osSpec);
    interstageMidHollow.prepare(osSpec);
    interstageUpperPush.prepare(osSpec);

    oversampledLowPass.prepare(osSpec);

    reset();
}

void ModernLeadPreamp::reset()
{
    oversampler.reset();

    inputHighPass.reset();
    finalLowPass.reset();

    postLowShelf.reset();
    postLowMid.reset();
    postBodyControl.reset();
    postMidHollow.reset();
    postUpperRecovery.reset();

    bassShelf.reset();
    midPeak.reset();
    trebleShelf.reset();
    trebleTiltLowShelf.reset();

    preHighPass.reset();
    preLowShelf.reset();
    preLowMid.reset();
    preMidHollow.reset();
    preUpperPush.reset();

    interstageHighPass.reset();
    interstageLowShelf.reset();
    interstageMidHollow.reset();
    interstageUpperPush.reset();

    oversampledLowPass.reset();
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
    postGainLinear = juce::jmax(0.0f, postGainLinear);

    updateBaseRateFilters(bassDb, midDb, trebleDb);
    updateOversampledFilters(drive01);

    juce::dsp::AudioBlock<float> inputBlock(buffer);

    // Front-end coupling only. No external dry path exists in this model.
    processFilter(inputHighPass, inputBlock);
    buffer.applyGain(calibration.inputTrim);

    processNonlinearPath(buffer, drive01);

    juce::dsp::AudioBlock<float> outputBlock(buffer);

    // Fixed Modern channel colour after the nonlinear path.
    processFilter(postLowShelf, outputBlock);
    processFilter(postLowMid, outputBlock);
    processFilter(postBodyControl, outputBlock);
    processFilter(postMidHollow, outputBlock);
    processFilter(postUpperRecovery, outputBlock);

    // User tone controls from the measured sweeps.
    processFilter(bassShelf, outputBlock);
    processFilter(midPeak, outputBlock);
    processFilter(trebleShelf, outputBlock);
    processFilter(trebleTiltLowShelf, outputBlock);

    processFilter(finalLowPass, outputBlock);

    buffer.applyGain(calibration.outputTrim * postGainLinear);
}

void ModernLeadPreamp::processNonlinearPath(juce::AudioBuffer<float> &buffer, float drive01)
{
    const auto d = shapeDrive(drive01, calibration.driveCurve);

    const auto stage1Drive =
        dbToGain(juce::jmap(d, calibration.stage1DriveMinDb, calibration.stage1DriveMaxDb));
    const auto stage2Drive =
        dbToGain(juce::jmap(d, calibration.stage2DriveMinDb, calibration.stage2DriveMaxDb));

    const auto stage1PositiveThreshold = juce::jmap(
        d, calibration.stage1PositiveThresholdMin, calibration.stage1PositiveThresholdMax);
    const auto stage1NegativeThreshold = juce::jmap(
        d, calibration.stage1NegativeThresholdMin, calibration.stage1NegativeThresholdMax);

    const auto stage2PositiveThreshold = juce::jmap(
        d, calibration.stage2PositiveThresholdMin, calibration.stage2PositiveThresholdMax);
    const auto stage2NegativeThreshold = juce::jmap(
        d, calibration.stage2NegativeThresholdMin, calibration.stage2NegativeThresholdMax);

    const auto edgeDrive = dbToGain(calibration.edgeDriveDb);

    juce::dsp::AudioBlock<float> baseBlock(buffer);
    auto osBlock = oversampler.processSamplesUp(baseBlock);

    processFilter(preHighPass, osBlock);
    processFilter(preLowShelf, osBlock);
    processFilter(preLowMid, osBlock);
    processFilter(preMidHollow, osBlock);
    processFilter(preUpperPush, osBlock);

    // Stage 1: asymmetric compression/limiting. It is still the main signal path,
    // not a parallel blend.
    for (size_t channel = 0; channel < osBlock.getNumChannels(); ++channel) {
        auto *samples = osBlock.getChannelPointer(channel);

        for (size_t i = 0; i < osBlock.getNumSamples(); ++i) {
            auto x = (samples[i] * stage1Drive) + calibration.stage1Asymmetry;
            x = solidStateKneeClamp(
                x,
                stage1PositiveThreshold,
                stage1NegativeThreshold,
                calibration.stage1Knee,
                calibration.stage1OverSlope);
            x -= calibration.stage1Asymmetry;

            samples[i] = x * calibration.stage1Trim;
        }
    }

    processFilter(interstageHighPass, osBlock);
    processFilter(interstageLowShelf, osBlock);
    processFilter(interstageMidHollow, osBlock);
    processFilter(interstageUpperPush, osBlock);

    // Stage 2: harder Modern clamp, followed by a small serial edge stage.
    for (size_t channel = 0; channel < osBlock.getNumChannels(); ++channel) {
        auto *samples = osBlock.getChannelPointer(channel);

        for (size_t i = 0; i < osBlock.getNumSamples(); ++i) {
            auto x = (samples[i] * stage2Drive) + calibration.stage2Asymmetry;
            x = solidStateKneeClamp(
                x,
                stage2PositiveThreshold,
                stage2NegativeThreshold,
                calibration.stage2Knee,
                calibration.stage2OverSlope);
            x -= calibration.stage2Asymmetry;
            x *= calibration.stage2Trim;

            // Third serial edge stage. This is deliberately NOT a blend with the
            // previous value. v10 brought back a perceived dry layer by blending the
            // less-clipped stage-2 signal with the edge. v11 keeps the v9 structural
            // fix: the edge stage REPLACES the previous signal path, but uses a softer
            // knee/slope calibration so it is tight distortion rather than square fuzz.
            auto edge = (x * edgeDrive) + (calibration.stage2Asymmetry * 0.35f);
            edge = solidStateKneeClamp(
                edge,
                calibration.edgePositiveThreshold,
                calibration.edgeNegativeThreshold,
                calibration.edgeKnee,
                calibration.edgeOverSlope);
            edge -= (calibration.stage2Asymmetry * 0.35f);

            samples[i] = edge * calibration.edgeOutputTrim;
        }
    }

    processFilter(oversampledLowPass, osBlock);

    oversampler.processSamplesDown(baseBlock);
}

void ModernLeadPreamp::updateBaseRateFilters(float bassDb, float midDb, float trebleDb)
{
    const auto bassControlDb =
        mapBipolarControlDb(bassDb, calibration.bassCutDb, calibration.bassBoostDb);

    const auto midControlDb =
        mapBipolarControlDb(midDb, calibration.midCutDb, calibration.midBoostDb);

    const auto trebleControlDb =
        mapBipolarControlDb(trebleDb, calibration.trebleCutDb, calibration.trebleBoostDb);

    const auto trebleTiltDb = mapBipolarControlDb(
        trebleDb, calibration.trebleTiltLowBoostAtMinDb, calibration.trebleTiltLowCutAtMaxDb);

    *inputHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        baseSampleRate, calibration.inputHighPassHz);

    *finalLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        baseSampleRate, calibration.finalLowPassHz);

    *postLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        baseSampleRate,
        calibration.postLowShelfHz,
        calibration.postLowShelfQ,
        dbToGain(calibration.postLowShelfDb));

    *postLowMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.postLowMidHz,
        calibration.postLowMidQ,
        dbToGain(calibration.postLowMidDb));

    *postBodyControl.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.postBodyControlHz,
        calibration.postBodyControlQ,
        dbToGain(calibration.postBodyControlDb));

    *postMidHollow.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.postMidHollowHz,
        calibration.postMidHollowQ,
        dbToGain(calibration.postMidHollowDb));

    *postUpperRecovery.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.postUpperRecoveryHz,
        calibration.postUpperRecoveryQ,
        dbToGain(calibration.postUpperRecoveryDb));

    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        baseSampleRate, calibration.bassShelfHz, calibration.bassShelfQ, dbToGain(bassControlDb));

    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate, calibration.midPeakHz, calibration.midPeakQ, dbToGain(midControlDb));

    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        baseSampleRate,
        calibration.trebleShelfHz,
        calibration.trebleShelfQ,
        dbToGain(trebleControlDb));

    *trebleTiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        baseSampleRate,
        calibration.trebleTiltLowShelfHz,
        calibration.trebleTiltLowShelfQ,
        dbToGain(trebleTiltDb));
}

void ModernLeadPreamp::updateOversampledFilters(float drive01)
{
    const auto d = shapeDrive(drive01, calibration.driveCurve);

    const auto midHollowDb =
        juce::jmap(d, calibration.preMidHollowMinDb, calibration.preMidHollowMaxDb);
    const auto upperPushDb =
        juce::jmap(d, calibration.preUpperPushMinDb, calibration.preUpperPushMaxDb);

    *preHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.preHighPassHz, calibration.preHighPassQ);

    *preLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        oversampledSampleRate,
        calibration.preLowShelfHz,
        calibration.preLowShelfQ,
        dbToGain(calibration.preLowShelfDb));

    *preLowMid.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.preLowMidHz,
        calibration.preLowMidQ,
        dbToGain(calibration.preLowMidDb));

    *preMidHollow.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.preMidHollowHz,
        calibration.preMidHollowQ,
        dbToGain(midHollowDb));

    *preUpperPush.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.preUpperPushHz,
        calibration.preUpperPushQ,
        dbToGain(upperPushDb));

    *interstageHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.interstageHighPassHz, calibration.interstageHighPassQ);

    *interstageLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        oversampledSampleRate,
        calibration.interstageLowShelfHz,
        calibration.interstageLowShelfQ,
        dbToGain(calibration.interstageLowShelfDb));

    *interstageMidHollow.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.interstageMidHollowHz,
        calibration.interstageMidHollowQ,
        dbToGain(calibration.interstageMidHollowDb));

    *interstageUpperPush.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        oversampledSampleRate,
        calibration.interstageUpperPushHz,
        calibration.interstageUpperPushQ,
        dbToGain(calibration.interstageUpperPushDb));

    *oversampledLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.oversampledLowPassHz);
}

float ModernLeadPreamp::dbToGain(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float ModernLeadPreamp::shapeDrive(float drive01, float curve)
{
    drive01 = juce::jlimit(0.0f, 1.0f, drive01);
    return std::pow(drive01, juce::jmax(0.05f, curve));
}

float ModernLeadPreamp::mapBipolarControlDb(
    float valueDb, float negativeExtremeDb, float positiveExtremeDb)
{
    valueDb = juce::jlimit(-12.0f, 12.0f, valueDb);

    if (valueDb < 0.0f)
        return juce::jmap(valueDb, -12.0f, 0.0f, negativeExtremeDb, 0.0f);

    return juce::jmap(valueDb, 0.0f, 12.0f, 0.0f, positiveExtremeDb);
}

float ModernLeadPreamp::solidStateKneeClamp(
    float x, float positiveThreshold, float negativeThreshold, float knee, float overSlope)
{
    positiveThreshold = juce::jmax(0.01f, positiveThreshold);
    negativeThreshold = juce::jmax(0.01f, negativeThreshold);
    knee = juce::jmax(0.001f, knee);
    overSlope = juce::jlimit(0.0f, 0.95f, overSlope);

    const auto clampPositive = [knee, overSlope](float threshold, float value) {
        const auto over = value - threshold;

        if (over <= 0.0f)
            return value;

        if (over < knee) {
            // Quadratic soft knee from slope 1.0 down to overSlope.
            const auto kneeReduction = (1.0f - overSlope) * over * over * 0.5f / knee;
            return threshold + over - kneeReduction;
        }

        const auto kneeEnd = threshold + (knee * (1.0f + overSlope) * 0.5f);
        return kneeEnd + ((over - knee) * overSlope);
    };

    if (x > positiveThreshold)
        return clampPositive(positiveThreshold, x);

    if (x < -negativeThreshold)
        return -clampPositive(negativeThreshold, -x);

    return x;
}

void ModernLeadPreamp::processFilter(Filter &filter, juce::dsp::AudioBlock<float> &block)
{
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
}
