#include "PluginProcessor.h"
#include "AmpPreamp/AmpPreamp.h"
#include "PluginEditor.h"

RedlineSSAudioProcessor::RedlineSSAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RedlineSSAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"inputGainDb", 1},
            "Input Gain",
            juce::NormalisableRange<float>{-24.0f, 24.0f, 0.01f},
            0.0f));

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"outputGainDb", 1},
            "Output Gain",
            juce::NormalisableRange<float>{-48.0f, 12.0f, 0.01f},
            -12.0f));

    return {params.begin(), params.end()};
}

void RedlineSSAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    currentNumChannels = juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());

    constexpr int oversamplingOrder = 2;
    // 1 = 2x
    // 2 = 4x
    // 3 = 8x

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(currentNumChannels),
        oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false);

    oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampler->reset();

    const auto oversampledRate =
        sampleRate * static_cast<double>(oversampler->getOversamplingFactor());

    ampPreamp.prepare(
        oversampledRate,
        samplesPerBlock * static_cast<int>(oversampler->getOversamplingFactor()),
        currentNumChannels);

    ampPreamp.getLeadPreGain().setPreGain(0.50f);
    ampPreamp.getLeadPreGain().setBrightAmount(0.45f);
    ampPreamp.getLeadPreGain().setOutputTrimDb(0.0f);

    ampPreamp.getQ5Q6GainStage().setDriveDb(28.0f);
    ampPreamp.getQ5Q6GainStage().setBias(0.10f);
    ampPreamp.getQ5Q6GainStage().setAsymmetry(0.45f);
    ampPreamp.getQ5Q6GainStage().setOutputTrimDb(-14.0f);
    ampPreamp.getQ5Q6GainStage().setPreHighPassHz(120.0f);
    ampPreamp.getQ5Q6GainStage().setPostLowPassHz(25000.0f);

    ampPreamp.getQ7Q8GainStage().setDriveDb(20.0f);
    ampPreamp.getQ7Q8GainStage().setBias(0.16f);
    ampPreamp.getQ7Q8GainStage().setAsymmetry(0.65f);
    ampPreamp.getQ7Q8GainStage().setCompression(0.0f);
    ampPreamp.getQ7Q8GainStage().setOutputTrimDb(-12.0f);
    ampPreamp.getQ7Q8GainStage().setPreHighPassHz(240.0f);
    ampPreamp.getQ7Q8GainStage().setPresenceLiftDb(1.5f);
    ampPreamp.getQ7Q8GainStage().setPostLowPassHz(22000.0f);

    ampPreamp.getBiasedClamp().setInputTrimDb(0.0f);
    ampPreamp.getBiasedClamp().setBias(0.20f);
    ampPreamp.getBiasedClamp().setPositiveLimit(0.85f);
    ampPreamp.getBiasedClamp().setNegativeLimit(0.45f);
    ampPreamp.getBiasedClamp().setKnee(0.04f);
    ampPreamp.getBiasedClamp().setOutputTrimDb(-3.0f);
    ampPreamp.getBiasedClamp().setPreHighPassHz(20.0f);
    ampPreamp.getBiasedClamp().setPostLowPassHz(15000.0f);

    ampPreamp.getRecoveryStage().setDriveDb(36.0f);
    ampPreamp.getRecoveryStage().setMakeupGainDb(12.0f);
    ampPreamp.getRecoveryStage().setBias(0.18f);
    ampPreamp.getRecoveryStage().setAsymmetry(0.60f);
    ampPreamp.getRecoveryStage().setKnee(0.02f);
    ampPreamp.getRecoveryStage().setOutputTrimDb(-18.0f);
    ampPreamp.getRecoveryStage().setPreHighPassHz(20.0f);
    ampPreamp.getRecoveryStage().setPostLowPassHz(18000.0f);
    ampPreamp.getRecoveryStage().setBodyFrequencyHz(150.0f);
    ampPreamp.getRecoveryStage().setBodyDb(3.0f);
    ampPreamp.getRecoveryStage().setDefizzFrequencyHz(6500.0f);
    ampPreamp.getRecoveryStage().setDefizzDb(-2.0f);

    ampPreamp.getQ11DriverStage().setInputTrimDb(0.0f);
    ampPreamp.getQ11DriverStage().setDriveDb(0.0f);
    ampPreamp.getQ11DriverStage().setBias(-0.08f);
    ampPreamp.getQ11DriverStage().setAsymmetry(0.45f);
    ampPreamp.getQ11DriverStage().setKnee(0.04f);
    ampPreamp.getQ11DriverStage().setOutputTrimDb(-3.0f);

    ampPreamp.getQ11DriverStage().setPreHighPassHz(30.0f);
    ampPreamp.getQ11DriverStage().setBodyFrequencyHz(160.0f);
    ampPreamp.getQ11DriverStage().setBodyDb(0.0f);
    ampPreamp.getQ11DriverStage().setMidShapeDb(0.0f);
    ampPreamp.getQ11DriverStage().setPresenceDb(0.0f);
    ampPreamp.getQ11DriverStage().setPostLowPassHz(15000.0f);
}

void RedlineSSAudioProcessor::releaseResources()
{
    ampPreamp.reset();
}

bool RedlineSSAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    const auto &mainInput = layouts.getMainInputChannelSet();
    const auto &mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainOutput)
        return false;

    if (mainOutput != juce::AudioChannelSet::mono() &&
        mainOutput != juce::AudioChannelSet::stereo()) {
        return false;
    }

    return true;
}

void RedlineSSAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    const auto inputGainDb = apvts.getRawParameterValue("inputGainDb")->load();
    const auto outputGainDb = apvts.getRawParameterValue("outputGainDb")->load();

    buffer.applyGain(juce::Decibels::decibelsToGain(inputGainDb));

    if (oversampler != nullptr) {
        juce::dsp::AudioBlock<float> block(buffer);

        auto oversampledBlock = oversampler->processSamplesUp(block);

        const int osChannels = static_cast<int>(oversampledBlock.getNumChannels());
        const int osSamples = static_cast<int>(oversampledBlock.getNumSamples());

        oversampledTempBuffer.setSize(osChannels, osSamples, false, false, true);

        for (int ch = 0; ch < osChannels; ++ch) {
            oversampledTempBuffer.copyFrom(
                ch, 0, oversampledBlock.getChannelPointer(static_cast<size_t>(ch)), osSamples);
        }

        ampPreamp.processUpTo(oversampledTempBuffer, AmpPreamp::TapPoint::AfterQ7Q8GainStage);

        for (int ch = 0; ch < osChannels; ++ch) {
            std::copy(
                oversampledTempBuffer.getReadPointer(ch),
                oversampledTempBuffer.getReadPointer(ch) + osSamples,
                oversampledBlock.getChannelPointer(static_cast<size_t>(ch)));
        }

        oversampler->processSamplesDown(block);
    } else {
        ampPreamp.process(buffer);
    }

    buffer.applyGain(juce::Decibels::decibelsToGain(outputGainDb));
}

juce::AudioProcessorEditor *RedlineSSAudioProcessor::createEditor()
{
    return new RedlineSSAudioProcessorEditor(*this);
}

bool RedlineSSAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String RedlineSSAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RedlineSSAudioProcessor::acceptsMidi() const
{
    return false;
}

bool RedlineSSAudioProcessor::producesMidi() const
{
    return false;
}

bool RedlineSSAudioProcessor::isMidiEffect() const
{
    return false;
}

double RedlineSSAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RedlineSSAudioProcessor::getNumPrograms()
{
    return 1;
}

int RedlineSSAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RedlineSSAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String RedlineSSAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void RedlineSSAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

void RedlineSSAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

void RedlineSSAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
        return;

    if (!xmlState->hasTagName(apvts.state.getType()))
        return;

    apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState &RedlineSSAudioProcessor::getApvts()
{
    return apvts;
}

const juce::AudioProcessorValueTreeState &RedlineSSAudioProcessor::getApvts() const
{
    return apvts;
}

// This creates new instances of the plugin.
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new RedlineSSAudioProcessor();
}
