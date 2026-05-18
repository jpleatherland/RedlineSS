#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Schematic/capture-guided macro model of the Bandit-style Modern Lead voice.
//
// Goal:
// Input conditioning
// -> first lead gain stage
// -> tighter coupling / treble shaping
// -> main lead gain stage
// -> firmer asymmetric diode clamp
// -> post-clamp recovery/makeup stage
// -> output buffer
// -> Modern voicing: tighter lows, low-mid scoop, upper-mid bite
// -> simplified lead tone controls
// -> post gain
//
// This is deliberately not a full circuit solver. It is a playable DSP model
// tuned from the Bandit schematic plus preamp-out capture data.

class ModernLeadPreamp {
  public:
    // Schematic-derived / parts-list-known values.
    // These are reference values rather than literal circuit-solver values.
    static constexpr float diodeForwardVoltage1N4148 = 0.62f;

    static constexpr float cap270pF = 270.0e-12f;
    static constexpr float cap470pF = 470.0e-12f;
    static constexpr float cap820pF = 820.0e-12f;
    static constexpr float cap1nF = 1000.0e-12f;
    static constexpr float cap1n5F = 1500.0e-12f;
    static constexpr float cap6n8F = 6800.0e-12f;

    static constexpr float cap15nF = 15.0e-9f;
    static constexpr float cap22nF = 22.0e-9f;
    static constexpr float cap47nF = 47.0e-9f;
    static constexpr float cap100nF = 100.0e-9f;
    static constexpr float cap220nF = 220.0e-9f;

    struct Calibration {
        // ---------- Input / global ----------
        // Keep enough low/lower-mid energy to actually hit the nonlinear stages.
        // The caller should not add another dirty-path HPF before this class.
        float inputHighPassHz = 70.0f;

        // Raw preamp out is brighter than a speaker, but should not be chainsaw fizz.
        float finalLowPassHz = 8500.0f;

        // The previous value was starving the whole lead path. Keep this high enough
        // that the main clamp is meaningfully driven, then control final level later.
        float inputTrim = 0.75f;

        // Overall preamp output trim before the caller's master gain.
        float outputTrim = 0.45f;

        // ---------- Pre-clipping Modern voicing ----------
        // Keep this subtle. The Modern scoop wants to happen mostly after clipping;
        // otherwise palm-mute body never reaches the clamp.
        float preClipLowMidScoopHz = 650.0f;
        float preClipLowMidScoopQ = 0.55f;
        float preClipLowMidScoopDb = -2.0f;

        float preClipUpperMidPushHz = 2600.0f;
        float preClipUpperMidPushQ = 0.85f;
        float preClipUpperMidPushDb = 3.0f;

        // ---------- First lead stage ----------
        // Mostly gain + slight asymmetric compression. Not intended to fuzz out.
        float firstLeadStageMaxGain = 6.0f;
        float firstLeadStagePositiveShape = 0.12f;
        float firstLeadStageNegativeShape = 0.10f;
        float firstLeadStageOutputTrim = 1.0f;
        float firstLeadStageLowPassHz = 8200.0f;

        // ---------- First coupling / interstage shape ----------
        // Tighter than Vintage, but not so high that the clipper loses its punch.
        float coupling1HighPassHz = 120.0f;

        // ---------- Main lead stage ----------
        // Drives the diode clamp harder than Vintage.
        float mainLeadStageMaxGain = 90.0f;
        float mainLeadStagePositiveShape = 0.85f;
        float mainLeadStageNegativeShape = 0.72f;
        float mainLeadStageOutputTrim = 0.95f;
        float mainLeadStageLowPassHz = 7600.0f;

        // ---------- Diode clamp ----------
        // Firmer, more aggressive asymmetric clamp for Modern.
        float diodePositiveThreshold = 0.28f;
        float diodeNegativeThreshold = 0.22f;
        float diodePositiveOverSlope = 0.06f;
        float diodeNegativeOverSlope = 0.05f;
        float diodeAsymmetry = 0.0f;
        float diodeOutputTrim = 1.15f;
        float postDiodeLowPassHz = 8200.0f;

        // ---------- Second coupling / recovery shape ----------
        float coupling2HighPassHz = 90.0f;

        // ---------- Post-limiter recovery ----------
        // This is not tube squish. It is a firmer transistor-ish makeup/recovery
        // stage so the clamp does not sound like a small, flat fuzz box.
        float postLimiterRecoveryGain = 3.0f;
        float postLimiterRecoveryPositiveShape = 0.18f;
        float postLimiterRecoveryNegativeShape = 0.14f;
        float postLimiterRecoveryOutputTrim = 0.55f;

        // ---------- Output buffer ----------
        // Very subtle. Mostly here to stop the output feeling mathematically sterile.
        float q11BufferDrive = 1.01f;
        float q11BufferMix = 0.01f;

        // ---------- Modern voicing ----------
        // Less vintage low-mid body, more upper-mid bite. This is post-clipping.
        float modernLowMidScoopHz = 700.0f;
        float modernLowMidScoopQ = 0.60f;
        float modernLowMidScoopDb = -7.0f;

        float modernUpperMidBiteHz = 3200.0f;
        float modernUpperMidBiteQ = 0.90f;
        float modernUpperMidBiteDb = 4.0f;

        float modernTopSofteningHz = 7200.0f;

        // ---------- Tone control approximation ----------
        // These are intentionally not +/-12 dB hi-fi EQ controls.
        // They approximate the usable sweep from the preamp-out captures.
        float bassShelfHz = 110.0f;
        float bassShelfQ = 0.65f;

        float midPeakHz = 720.0f;
        float midPeakQ = 0.75f;

        float trebleShelfHz = 2800.0f;
        float trebleShelfQ = 0.70f;
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
    static float mapControlDb(float valueDb, float minDb, float maxDb);

    static float modernClip(
        float x,
        float drive,
        float positiveThreshold,
        float negativeThreshold,
        float positiveOverSlope,
        float negativeOverSlope,
        float asymmetry);

    static float
    transistorPair(float x, float gain, float positiveShape, float negativeShape, float outputTrim);

    static float mix(float dry, float wet, float wetMix);

    static float rcCutoffHz(float resistanceOhms, float capacitanceFarads)
    {
        return 1.0f / (juce::MathConstants<float>::twoPi * resistanceOhms * capacitanceFarads);
    }

    void updateBaseRateFilters(float bassDb, float midDb, float trebleDb);
    void updateOversampledFilters();

    void processNonlinearBlock(juce::dsp::AudioBlock<float> &block, float drive01);

    static void processFilter(Filter &filter, juce::dsp::AudioBlock<float> &block);

    Calibration calibration;

    double baseSampleRate = 44100.0;
    double oversampledSampleRate = 176400.0;

    juce::dsp::Oversampling<float> oversampler;

    Filter inputHighPass;
    Filter finalLowPass;

    Filter preClipLowMidScoop;
    Filter preClipUpperMidPush;

    Filter firstLeadStageLowPass;
    Filter coupling1HighPass;

    Filter mainLeadStageLowPass;
    Filter postDiodeLowPass;
    Filter coupling2HighPass;

    Filter modernLowMidScoop;
    Filter modernUpperMidBite;
    Filter modernTopSoftening;

    Filter bassShelf;
    Filter midPeak;
    Filter trebleShelf;
};
