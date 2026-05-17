#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Schematic-guided macro model of the Bandit-style Vintage Lead preamp path.
//
// Goal:
// Input conditioning
// -> Q5/Q6-ish first gain pair
// -> coupling / treble shaping
// -> Q7/Q8-ish main gain pair
// -> 1N4148-ish asymmetric diode clamp
// -> Q9/Q10-ish recovery / extra colour
// -> Q11-ish buffer
// -> Vintage voicing
// -> simplified lead tone controls
// -> post gain
//
// This is deliberately not a full circuit solver. Values in Calibration are
// named after the schematic functions they are standing in for, so we can tune
// them by ear against the real amp.

class VintageLeadPreamp {
  public:
    // Schematic-derived / parts-list-known values.
    // These are not all used as literal one-pole cutoff values yet,
    // because loading and transistor impedances matter, but they give us
    // grounded starting points.
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
        // Front-end high-pass before distortion.
        // The schematic has coupling caps much lower than this in places, but
        // guitar amp gain channels often feel better with a practical low-end
        // tightening point before nonlinear stages.
        float inputHighPassHz = 55.0f;

        // Final anti-fizz safety filter. External IR still does most of the
        // real speaker work.
        float finalLowPassHz = 7800.0f;

        // Overall preamp output trim before the caller's master gain.
        float outputTrim = 0.75f;

        // ---------- Q5/Q6 approximation ----------
        // Schematic cue:
        // Q5/Q6 2N/PNP-style transistor pair around the early Lead path.
        // There are small caps and emitter bypass parts shaping gain/fizz.
        float firstStageLeadMaxGain = 7.0f;
        float q5q6PositiveShape = 0.38f;
        float q5q6NegativeShape = 0.32f;
        float q5q6OutputTrim = 0.82f;

        // Small-cap treble limiting around first lead gain area.
        // This is standing in for the pF/nF local HF shaping around that area.
        float q5q6LowPassHz = 7600.0f;

        // ---------- First coupling / interstage shape ----------
        // Several actual coupling caps calculate much lower than this if taken
        // literally against very high impedances, but the loaded/effective audio
        // result is tighter than "2 Hz full bass into distortion".
        float coupling1HighPassHz = 75.0f;

        // ---------- Q7/Q8 approximation ----------
        // Schematic cue:
        // Q7/Q8 form another transistor gain section before/around the lead
        // diode clamp network.
        float mainStageLeadMaxGain = 15.0f;
        float q7q8PositiveShape = 0.52f;
        float q7q8NegativeShape = 0.44f;
        float q7q8OutputTrim = 0.82f;

        // More local HF softening before the diode network.
        float q7q8LowPassHz = 6900.0f;

        // ---------- Diode clamp approximation ----------
        // Schematic cue:
        // CR7/CR8/CR9 and nearby biasing create diode limiting/asymmetry.
        // The exact switching state and surrounding transistor impedances matter,
        // so these thresholds are intentionally exposed.
        float diodePositiveThreshold = 1.05f;
        float diodeNegativeThreshold = 0.78f;
        float diodeKnee = 4.2f;
        float diodeOutputTrim = 0.95f;

        // Tightening after the clamp. This removes diode hash before recovery.
        float postDiodeLowPassHz = 7400.0f;

        // ---------- Second coupling / recovery shape ----------
        float coupling2HighPassHz = 85.0f;

        // ---------- Q9/Q10 recovery approximation ----------
        // Schematic cue:
        // Q9/Q10 provide recovery / further transistor colour after the diode
        // network. This should not be as brutal as the current generic Stage 3.
        float postLimiterRecoveryGain = 2.2f;
        float q9q10PositiveShape = 0.18f;
        float q9q10NegativeShape = 0.15f;
        float q9q10OutputTrim = 1.00f;

        // ---------- Q11 / lead output buffer ----------
        // Schematic cue:
        // Q11 933 area is treated mostly as a buffer / low impedance driver.
        // This adds tiny softening only when driven hard.
        float q11BufferDrive = 1.15f;
        float q11BufferMix = 0.01f;

        // ---------- Vintage voicing approximation ----------
        // Schematic goal:
        // Vintage is the less scooped / more classic lead voicing versus Modern.
        // This fixed shaping happens before the user tone controls.
        float vintageLowMidDipHz = 420.0f;
        float vintageLowMidDipQ = 0.75f;
        float vintageLowMidDipDb = -1.8f;

        float vintageUpperMidPushHz = 1250.0f;
        float vintageUpperMidPushQ = 0.85f;
        float vintageUpperMidPushDb = 2.2f;

        // Gentle pre-tone-stack treble softening.
        float vintageTopSofteningHz = 5200.0f;

        // ---------- Tone control approximation ----------
        // These are not exact passive network values yet. They give us useful
        // amp-like control while the preamp gain structure is tuned.
        float bassShelfHz = 120.0f;
        float bassShelfQ = 0.65f;

        float midPeakHz = 720.0f;
        float midPeakQ = 0.75f;

        float trebleShelfHz = 2600.0f;
        float trebleShelfQ = 0.70f;
    };

    VintageLeadPreamp();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    // drive01:
    //   0.0 -> nearly clean / pushed transistor path
    //   1.0 -> full Vintage Lead gain
    //
    // bassDb, midDb, trebleDb:
    //   Current approximation tone controls. Feed your existing UI values here.
    //
    // postGainLinear:
    //   Caller-facing channel/master level.
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
    transistorPair(float x, float gain, float positiveShape, float negativeShape, float outputTrim);
    static float diodeClamp(float x, float positiveThreshold, float negativeThreshold, float knee);
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

    Filter q5q6LowPass;
    Filter coupling1HighPass;

    Filter q7q8LowPass;
    Filter postDiodeLowPass;
    Filter coupling2HighPass;

    Filter vintageLowMidDip;
    Filter vintageUpperMidPush;
    Filter vintageTopSoftening;

    Filter bassShelf;
    Filter midPeak;
    Filter trebleShelf;
};
