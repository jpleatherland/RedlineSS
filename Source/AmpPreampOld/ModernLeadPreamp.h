#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Measurement-guided macro model for the Peavey Bandit 112 Lead / Modern voice.
// v11 no-dry-tight: serial dirty path only, no edge blend, less square-wave fuzz.
//
// This is intentionally NOT a tube-style tanh saturator and NOT a clean blend.
// The whole signal goes through the nonlinear path. Low/body is controlled before
// the hard damage, then recovered after clipping with EQ/recovery gain. This keeps
// the low end from square-waving into fuzz, without leaving an audible DI layer.
//
// Signal shape:
// input coupling
// -> oversampled pre-emphasis / low control / mid hollow / upper push
// -> stage 1: firm asymmetric transistor-style compression
// -> interstage voicing
// -> stage 2: harder solid-state clamp + small serial edge densifier
// -> fixed Modern output voice
// -> measured-ish Bass/Mid/Treble controls
// -> final low-pass / output trim

class ModernLeadPreamp {
  public:
    struct Calibration {
        // ---------- Global ----------
        float inputHighPassHz = 92.0f;
        float finalLowPassHz = 10100.0f;
        float inputTrim = 2.85f;
        float outputTrim = 0.31f;

        // Incoming drive01 is expected to be gain1 / 10.0f.
        // Kept from previous versions so your current AmpPreamp call can stay unchanged.
        float driveCurve = 0.50f;

        // ---------- Oversampled pre-shape ----------
        // Lows are not clean-blended; they still go through the nonlinear stages.
        // They are simply held back before the harder clamp so they do not become fuzz wool.
        float preHighPassHz = 128.0f;
        float preHighPassQ = 0.70f;

        float preLowShelfHz = 180.0f;
        float preLowShelfQ = 0.70f;
        float preLowShelfDb = -5.6f;

        float preLowMidHz = 330.0f;
        float preLowMidQ = 0.78f;
        float preLowMidDb = -1.0f;

        float preMidHollowHz = 970.0f;
        float preMidHollowQ = 0.82f;
        float preMidHollowMinDb = -2.0f;
        float preMidHollowMaxDb = -4.8f;

        float preUpperPushHz = 3100.0f;
        float preUpperPushQ = 0.92f;
        float preUpperPushMinDb = 3.2f;
        float preUpperPushMaxDb = 5.8f;

        // ---------- Stage 1: transistor-style compression ----------
        // This stage gives density and gain without doing the entire clipping job.
        float stage1DriveMinDb = 32.0f;
        float stage1DriveMaxDb = 54.0f;
        float stage1PositiveThresholdMin = 0.26f;
        float stage1PositiveThresholdMax = 0.125f;
        float stage1NegativeThresholdMin = 0.225f;
        float stage1NegativeThresholdMax = 0.105f;
        float stage1Knee = 0.060f;
        float stage1OverSlope = 0.055f;
        float stage1Asymmetry = 0.006f;
        float stage1Trim = 0.50f;

        // ---------- Interstage voicing ----------
        float interstageHighPassHz = 170.0f;
        float interstageHighPassQ = 0.70f;

        float interstageLowShelfHz = 210.0f;
        float interstageLowShelfQ = 0.70f;
        float interstageLowShelfDb = -5.2f;

        float interstageMidHollowHz = 1040.0f;
        float interstageMidHollowQ = 0.76f;
        float interstageMidHollowDb = -3.8f;

        float interstageUpperPushHz = 3300.0f;
        float interstageUpperPushQ = 0.82f;
        float interstageUpperPushDb = 3.7f;

        // ---------- Stage 2: hard Modern clamp ----------
        float stage2DriveMinDb = 41.0f;
        float stage2DriveMaxDb = 68.0f;
        float stage2PositiveThresholdMin = 0.115f;
        float stage2PositiveThresholdMax = 0.042f;
        float stage2NegativeThresholdMin = 0.095f;
        float stage2NegativeThresholdMax = 0.035f;
        float stage2Knee = 0.038f;
        float stage2OverSlope = 0.028f;
        float stage2Asymmetry = 0.004f;
        float stage2Trim = 0.58f;

        // Small serial edge stage. This is not a parallel clean blend; it is fed by
        // the already-clamped signal to add the crispy solid-state edge.
        float edgeDriveDb = 11.5f;
        float edgePositiveThreshold = 0.110f;
        float edgeNegativeThreshold = 0.092f;
        float edgeKnee = 0.044f;
        float edgeOverSlope = 0.026f;
        float edgeMix = 1.0f;
        float edgeOutputTrim = 0.78f;

        float oversampledLowPassHz = 9300.0f;

        // ---------- Fixed Modern post-voice ----------
        // Low/low-mid support is recovered after clipping, not clean-blended.
        float postLowShelfHz = 145.0f;
        float postLowShelfQ = 0.68f;
        float postLowShelfDb = -3.4f;

        float postLowMidHz = 245.0f;
        float postLowMidQ = 0.82f;
        float postLowMidDb = -2.7f;

        float postBodyControlHz = 430.0f;
        float postBodyControlQ = 0.78f;
        float postBodyControlDb = -7.2f;

        float postMidHollowHz = 1010.0f;
        float postMidHollowQ = 0.78f;
        float postMidHollowDb = -4.8f;

        float postUpperRecoveryHz = 3400.0f;
        float postUpperRecoveryQ = 0.82f;
        float postUpperRecoveryDb = 4.2f;

        // ---------- Tone controls ----------
        // UI controls are expected as approximately -12..+12 dB, with 0 = noon.
        float bassShelfHz = 165.0f;
        float bassShelfQ = 0.70f;
        float bassCutDb = -5.8f;
        float bassBoostDb = 4.4f;

        float midPeakHz = 1000.0f;
        float midPeakQ = 0.78f;
        float midCutDb = -7.8f;
        float midBoostDb = 3.0f;

        float trebleShelfHz = 2450.0f;
        float trebleShelfQ = 0.68f;
        float trebleCutDb = -5.5f;
        float trebleBoostDb = 3.8f;

        float trebleTiltLowShelfHz = 520.0f;
        float trebleTiltLowShelfQ = 0.60f;
        float trebleTiltLowBoostAtMinDb = 4.0f;
        float trebleTiltLowCutAtMaxDb = -1.3f;
    };

    ModernLeadPreamp();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void processBlock(
        juce::AudioBuffer<float> &buffer,
        float drive01,
        float bassDb,
        float midDb,
        float trebleDb,
        float postGainLinear);

    Calibration &getCalibration() noexcept
    {
        return calibration;
    }
    const Calibration &getCalibration() const noexcept
    {
        return calibration;
    }

  private:
    using Filter = juce::dsp::
        ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;

    static float dbToGain(float db);
    static float shapeDrive(float drive01, float curve);
    static float
    mapBipolarControlDb(float valueDb, float negativeExtremeDb, float positiveExtremeDb);
    static float solidStateKneeClamp(
        float x, float positiveThreshold, float negativeThreshold, float knee, float overSlope);

    static void processFilter(Filter &filter, juce::dsp::AudioBlock<float> &block);

    void updateBaseRateFilters(float bassDb, float midDb, float trebleDb);
    void updateOversampledFilters(float drive01);
    void processNonlinearPath(juce::AudioBuffer<float> &buffer, float drive01);

    Calibration calibration;

    double baseSampleRate = 44100.0;
    double oversampledSampleRate = 176400.0;
    int preparedChannels = 2;
    int preparedBlockSize = 512;

    juce::dsp::Oversampling<float> oversampler;

    // Base-rate filters.
    Filter inputHighPass;
    Filter finalLowPass;

    Filter postLowShelf;
    Filter postLowMid;
    Filter postBodyControl;
    Filter postMidHollow;
    Filter postUpperRecovery;

    Filter bassShelf;
    Filter midPeak;
    Filter trebleShelf;
    Filter trebleTiltLowShelf;

    // Oversampled nonlinear-path filters.
    Filter preHighPass;
    Filter preLowShelf;
    Filter preLowMid;
    Filter preMidHollow;
    Filter preUpperPush;

    Filter interstageHighPass;
    Filter interstageLowShelf;
    Filter interstageMidHollow;
    Filter interstageUpperPush;

    Filter oversampledLowPass;
};
