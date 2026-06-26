#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class RedlineSSAudioProcessorEditor final : public juce::AudioProcessorEditor {
  public:
    explicit RedlineSSAudioProcessorEditor(RedlineSSAudioProcessor &processor);
    ~RedlineSSAudioProcessorEditor() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    RedlineSSAudioProcessor &audioProcessor;

    juce::Label titleLabel;

    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    std::unique_ptr<SliderAttachment> inputGainAttachment;

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    void configureSlider(juce::Slider &slider);
    void configureLabel(juce::Label &label, const juce::String &text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RedlineSSAudioProcessorEditor)
};
