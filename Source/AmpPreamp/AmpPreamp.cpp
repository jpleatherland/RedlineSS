#include "AmpPreamp.h"
#include "BiasedClamp.h"

void AmpPreamp::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    inputConditioning.prepare(sampleRate, samplesPerBlock, numChannels);
    leadPreGain.prepare(sampleRate, samplesPerBlock, numChannels);
    q5q6GainStage.prepare(sampleRate, samplesPerBlock, numChannels);
    q7q8GainStage.prepare(sampleRate, samplesPerBlock, numChannels);
    biasedClamp.prepare(sampleRate, samplesPerBlock, numChannels);
    recoveryStage.prepare(sampleRate, samplesPerBlock, numChannels);
    q11DriverStage.prepare(sampleRate, samplesPerBlock, numChannels);
}

void AmpPreamp::reset()
{
    inputConditioning.reset();
    leadPreGain.reset();
    q5q6GainStage.reset();
    q7q8GainStage.reset();
    biasedClamp.reset();
    recoveryStage.reset();
    q11DriverStage.reset();
}

void AmpPreamp::process(juce::AudioBuffer<float> &buffer)
{
    processUpTo(buffer, TapPoint::Output);
}

void AmpPreamp::processUpTo(juce::AudioBuffer<float> &buffer, TapPoint tapPoint)
{
    if (buffer.getNumSamples() == 0)
        return;

    switch (tapPoint) {
    case TapPoint::Input:
        return;

    case TapPoint::AfterInputConditioning:
        inputConditioning.process(buffer);
        return;

    case TapPoint::AfterLeadPreGain:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        return;

    case TapPoint::AfterQ5Q6GainStage:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        return;

    case TapPoint::AfterQ7Q8GainStage:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        q7q8GainStage.process(buffer);
        return;

    case TapPoint::AfterBiasedClamp:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        q7q8GainStage.process(buffer);
        biasedClamp.process(buffer);
        return;

    case TapPoint::AfterRecovery:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        q7q8GainStage.process(buffer);
        biasedClamp.process(buffer);
        recoveryStage.process(buffer);
        return;

    case TapPoint::AfterDriver:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        q7q8GainStage.process(buffer);
        biasedClamp.process(buffer);
        recoveryStage.process(buffer);
        q11DriverStage.process(buffer);
        return;

    case TapPoint::Output:
        inputConditioning.process(buffer);
        leadPreGain.process(buffer);
        q5q6GainStage.process(buffer);
        q7q8GainStage.process(buffer);
        biasedClamp.process(buffer);
        recoveryStage.process(buffer);

        // Later stages will go here:
        //
        // driver.process(buffer);
        // toneStack.process(buffer);
        // postGain.process(buffer);

        return;
    }

    jassertfalse;
}

InputConditioning &AmpPreamp::getInputConditioning()
{
    return inputConditioning;
}

const InputConditioning &AmpPreamp::getInputConditioning() const
{
    return inputConditioning;
}

LeadPreGain &AmpPreamp::getLeadPreGain()
{
    return leadPreGain;
}

const LeadPreGain &AmpPreamp::getLeadPreGain() const
{
    return leadPreGain;
}

Q5Q6GainStage &AmpPreamp::getQ5Q6GainStage()
{
    return q5q6GainStage;
}

const Q5Q6GainStage &AmpPreamp::getQ5Q6GainStage() const
{
    return q5q6GainStage;
}

Q7Q8GainStage &AmpPreamp::getQ7Q8GainStage()
{
    return q7q8GainStage;
}

const Q7Q8GainStage &AmpPreamp::getQ7Q8GainStage() const
{
    return q7q8GainStage;
}

BiasedClamp &AmpPreamp::getBiasedClamp()
{
    return biasedClamp;
}

const BiasedClamp &AmpPreamp::getBiasedClamp() const
{
    return biasedClamp;
}

RecoveryStage &AmpPreamp::getRecoveryStage()
{
    return recoveryStage;
}

const RecoveryStage &AmpPreamp::getRecoveryStage() const
{
    return recoveryStage;
}

Q11DriverStage &AmpPreamp::getQ11DriverStage()
{
    return q11DriverStage;
}

const Q11DriverStage &AmpPreamp::getQ11DriverStage() const
{
    return q11DriverStage;
}
