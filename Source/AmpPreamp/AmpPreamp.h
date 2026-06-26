#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "BiasedClamp.h"
#include "InputConditioning.h"
#include "LeadPreGain.h"
#include "Q11DriverStage.h"
#include "Q5Q6GainStage.h"
#include "Q7Q8GainStage.h"
#include "RecoveryStage.h"

class AmpPreamp {
  public:
    enum class TapPoint {
        Input,
        AfterInputConditioning,
        AfterLeadPreGain,
        AfterQ5Q6GainStage,
        AfterQ7Q8GainStage,
        AfterBiasedClamp,
        AfterRecovery,
        AfterDriver,
        // AfterToneStack,
        // AfterPostGain,

        Output
    };

    AmpPreamp() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void process(juce::AudioBuffer<float> &buffer);

    void processUpTo(juce::AudioBuffer<float> &buffer, TapPoint tapPoint);

    InputConditioning &getInputConditioning();
    const InputConditioning &getInputConditioning() const;

    LeadPreGain &getLeadPreGain();
    const LeadPreGain &getLeadPreGain() const;

    Q5Q6GainStage &getQ5Q6GainStage();
    const Q5Q6GainStage &getQ5Q6GainStage() const;

    Q7Q8GainStage &getQ7Q8GainStage();
    const Q7Q8GainStage &getQ7Q8GainStage() const;

    BiasedClamp &getBiasedClamp();
    const BiasedClamp &getBiasedClamp() const;

    RecoveryStage &getRecoveryStage();
    const RecoveryStage &getRecoveryStage() const;

    Q11DriverStage &getQ11DriverStage();
    const Q11DriverStage &getQ11DriverStage() const;

  private:
    InputConditioning inputConditioning;
    LeadPreGain leadPreGain;
    Q5Q6GainStage q5q6GainStage;
    Q7Q8GainStage q7q8GainStage;
    BiasedClamp biasedClamp;
    RecoveryStage recoveryStage;
    Q11DriverStage q11DriverStage;
};
