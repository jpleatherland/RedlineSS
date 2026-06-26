#include "VintageLeadPreamp.h"

#include <cmath>

VintageLeadPreamp::VintageLeadPreamp()
    : oversampler(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
}

void VintageLeadPreamp::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::ignoreUnused(numChannels);

    baseSampleRate = sampleRate;
    oversampledSampleRate = sampleRate * oversampler.getOversamplingFactor();

    oversampler.initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler.reset();

    juce::dsp::ProcessSpec baseSpec;
    baseSpec.sampleRate = baseSampleRate;
    baseSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    baseSpec.numChannels = 2;

    inputHighPass.prepare(baseSpec);
    finalLowPass.prepare(baseSpec);

    vintageLowMidDip.prepare(baseSpec);
    vintageUpperMidPush.prepare(baseSpec);
    vintageTopSoftening.prepare(baseSpec);

    bassShelf.prepare(baseSpec);
    midPeak.prepare(baseSpec);
    trebleShelf.prepare(baseSpec);

    juce::dsp::ProcessSpec osSpec;
    osSpec.sampleRate = oversampledSampleRate;
    osSpec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock * oversampler.getOversamplingFactor());
    osSpec.numChannels = 2;

    q5q6LowPass.prepare(osSpec);
    coupling1HighPass.prepare(osSpec);

    q7q8LowPass.prepare(osSpec);
    postDiodeLowPass.prepare(osSpec);
    coupling2HighPass.prepare(osSpec);

    reset();
}

void VintageLeadPreamp::reset()
{
    oversampler.reset();

    inputHighPass.reset();
    finalLowPass.reset();

    q5q6LowPass.reset();
    coupling1HighPass.reset();

    q7q8LowPass.reset();
    postDiodeLowPass.reset();
    coupling2HighPass.reset();

    vintageLowMidDip.reset();
    vintageUpperMidPush.reset();
    vintageTopSoftening.reset();

    bassShelf.reset();
    midPeak.reset();
    trebleShelf.reset();
}

void VintageLeadPreamp::processBlock(
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

    // Schematic stand-in:
    // input coupling / front-end low cut before lead gain.
    processFilter(inputHighPass, block);

    // Nonlinear transistor + diode sections are oversampled.
    auto osBlock = oversampler.processSamplesUp(block);
    processNonlinearBlock(osBlock, drive01);
    oversampler.processSamplesDown(block);

    // Schematic stand-in:
    // fixed Vintage voice before user tone controls.
    processFilter(vintageLowMidDip, block);
    processFilter(vintageUpperMidPush, block);
    processFilter(vintageTopSoftening, block);

    // Current practical tone stack approximation.
    processFilter(bassShelf, block);
    processFilter(midPeak, block);
    processFilter(trebleShelf, block);

    // External IR should do most speaker roll-off, but this prevents raw clip hash.
    processFilter(finalLowPass, block);

    buffer.applyGain(calibration.outputTrim * postGainLinear);
}

void VintageLeadPreamp::processNonlinearBlock(juce::dsp::AudioBlock<float> &block, float drive01)
{
    const auto d1 = shapeDrive(drive01, 1.15f);
    const auto d2 = shapeDrive(drive01, 1.35f);
    const auto d3 = shapeDrive(drive01, 2.20f);

    const auto firstStageGain = juce::jmap(d1, 1.0f, calibration.firstStageLeadMaxGain);
    const auto mainStageGain = juce::jmap(d2, 1.0f, calibration.mainStageLeadMaxGain);
    const auto postLimiterRecoveryGain = juce::jmap(d3, 1.0f, calibration.postLimiterRecoveryGain);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            // Q5/Q6-ish first lead gain pair.
            x = transistorPair(
                x,
                firstStageGain,
                calibration.q5q6PositiveShape,
                calibration.q5q6NegativeShape,
                calibration.q5q6OutputTrim);

            samples[i] = x;
        }
    }

    processFilter(q5q6LowPass, block);
    processFilter(coupling1HighPass, block);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            // Q7/Q8-ish main lead gain pair.
            x = transistorPair(
                x,
                mainStageGain,
                calibration.q7q8PositiveShape,
                calibration.q7q8NegativeShape,
                calibration.q7q8OutputTrim);

            samples[i] = x;
        }
    }

    processFilter(q7q8LowPass, block);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            // 1N4148-ish asymmetric diode limiter.
            x = diodeClamp(
                x,
                calibration.diodePositiveThreshold,
                calibration.diodeNegativeThreshold,
                calibration.diodeKnee);

            samples[i] = x * calibration.diodeOutputTrim;
        }
    }

    processFilter(postDiodeLowPass, block);
    processFilter(coupling2HighPass, block);

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel) {
        auto *samples = block.getChannelPointer(channel);

        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = samples[i];

            // Q9/Q10-ish recovery / extra transistor colour.
            x = transistorPair(
                x,
                postLimiterRecoveryGain,
                calibration.q9q10PositiveShape,
                calibration.q9q10NegativeShape,
                calibration.q9q10OutputTrim);

            // Q11-ish output buffer. Mostly clean, slightly soft if slammed.
            const auto buffered =
                std::tanh(x * calibration.q11BufferDrive) / std::tanh(calibration.q11BufferDrive);

            x = mix(x, buffered, calibration.q11BufferMix);

            samples[i] = x;
        }
    }
}

static float mapControlDb(float valueDb, float minDb, float maxDb)
{
    const auto normalised =
        juce::jmap(juce::jlimit(-12.0f, 12.0f, valueDb), -12.0f, 12.0f, 0.0f, 1.0f);

    return juce::jmap(normalised, minDb, maxDb);
}

void VintageLeadPreamp::updateBaseRateFilters(float bassDb, float midDb, float trebleDb)
{
    const auto banditBassDb = mapControlDb(bassDb, -7.0f, 5.0f);

    const auto banditMidDb = mapControlDb(midDb, -7.0f, 3.0f);

    const auto banditTrebleDb = mapControlDb(trebleDb, -8.0f, 5.0f);

    *inputHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        baseSampleRate, calibration.inputHighPassHz);

    *finalLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        baseSampleRate, calibration.finalLowPassHz);

    *vintageLowMidDip.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.vintageLowMidDipHz,
        calibration.vintageLowMidDipQ,
        dbToGain(calibration.vintageLowMidDipDb));

    *vintageUpperMidPush.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate,
        calibration.vintageUpperMidPushHz,
        calibration.vintageUpperMidPushQ,
        dbToGain(calibration.vintageUpperMidPushDb));

    *vintageTopSoftening.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        baseSampleRate, calibration.vintageTopSofteningHz);

    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        baseSampleRate, calibration.bassShelfHz, calibration.bassShelfQ, dbToGain(banditBassDb));

    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        baseSampleRate, calibration.midPeakHz, calibration.midPeakQ, dbToGain(banditMidDb));

    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        baseSampleRate,
        calibration.trebleShelfHz,
        calibration.trebleShelfQ,
        dbToGain(banditTrebleDb));
}

void VintageLeadPreamp::updateOversampledFilters()
{
    *q5q6LowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.q5q6LowPassHz);

    *coupling1HighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.coupling1HighPassHz);

    *q7q8LowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.q7q8LowPassHz);

    *postDiodeLowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        oversampledSampleRate, calibration.postDiodeLowPassHz);

    *coupling2HighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        oversampledSampleRate, calibration.coupling2HighPassHz);
}

float VintageLeadPreamp::dbToGain(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

float VintageLeadPreamp::shapeDrive(float drive01, float curve)
{
    drive01 = juce::jlimit(0.0f, 1.0f, drive01);
    return std::pow(drive01, curve);
}

float VintageLeadPreamp::transistorPair(
    float x, float gain, float positiveShape, float negativeShape, float outputTrim)
{
    x *= gain;

    const auto shape = x >= 0.0f ? positiveShape : negativeShape;

    // Softer BJT-ish compression.
    // This bends the waveform without immediately flattening it into fuzz.
    const auto absX = std::abs(x);
    const auto compressed = x / std::sqrt(1.0f + shape * absX * absX);

    return compressed * outputTrim;
}

float VintageLeadPreamp::diodeClamp(
    float x, float positiveThreshold, float negativeThreshold, float knee)
{
    positiveThreshold = juce::jmax(0.05f, positiveThreshold);
    negativeThreshold = juce::jmax(0.05f, negativeThreshold);
    knee = juce::jmax(0.1f, knee);

    if (x > positiveThreshold) {
        const auto over = x - positiveThreshold;
        return positiveThreshold + std::tanh(over * knee) * 0.22f;
    }

    if (x < -negativeThreshold) {
        const auto over = -x - negativeThreshold;
        return -negativeThreshold - std::tanh(over * knee) * 0.22f;
    }

    return x;
}

float VintageLeadPreamp::mix(float dry, float wet, float wetMix)
{
    wetMix = juce::jlimit(0.0f, 1.0f, wetMix);
    return dry + wetMix * (wet - dry);
}

void VintageLeadPreamp::processFilter(Filter &filter, juce::dsp::AudioBlock<float> &block)
{
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
}
