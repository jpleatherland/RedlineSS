#include "PluginEditor.h"

RedlineSSAudioProcessorEditor::RedlineSSAudioProcessorEditor(RedlineSSAudioProcessor &processor)
    : AudioProcessorEditor(&processor), audioProcessor(processor)
{
    setSize(520, 260);
    setResizable(true, true);
    setResizeLimits(420, 220, 1200, 700);

    titleLabel.setText("RedlineSS", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    configureSlider(inputGainSlider);
    configureLabel(inputGainLabel, "Input");

    configureSlider(outputGainSlider);
    configureLabel(outputGainLabel, "Output");

    addAndMakeVisible(inputGainSlider);
    addAndMakeVisible(inputGainLabel);

    addAndMakeVisible(outputGainSlider);
    addAndMakeVisible(outputGainLabel);

    inputGainAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.getApvts(), "inputGainDb", inputGainSlider);

    outputGainAttachment = std::make_unique<SliderAttachment>(
        audioProcessor.getApvts(), "outputGainDb", outputGainSlider);
}

void RedlineSSAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour::fromRGB(20, 20, 22));

    auto bounds = getLocalBounds().toFloat().reduced(12.0f);

    g.setColour(juce::Colour::fromRGB(150, 25, 28));
    g.drawRoundedRectangle(bounds, 12.0f, 2.0f);

    g.setColour(juce::Colour::fromRGB(35, 35, 38));
    g.fillRoundedRectangle(bounds.reduced(4.0f), 10.0f);
}

void RedlineSSAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(24);

    titleLabel.setBounds(bounds.removeFromTop(48));

    bounds.removeFromTop(16);

    auto controlsArea = bounds.removeFromTop(150);
    auto controlWidth = controlsArea.getWidth() / 2;

    auto inputArea = controlsArea.removeFromLeft(controlWidth).reduced(18, 0);
    auto outputArea = controlsArea.reduced(18, 0);

    inputGainSlider.setBounds(inputArea.removeFromTop(110));
    inputGainLabel.setBounds(inputArea.removeFromTop(28));

    outputGainSlider.setBounds(outputArea.removeFromTop(110));
    outputGainLabel.setBounds(outputArea.removeFromTop(28));
}

void RedlineSSAudioProcessorEditor::configureSlider(juce::Slider &slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);

    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(190, 35, 38));

    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(65, 65, 70));

    slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(235, 235, 235));

    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);

    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(25, 25, 28));

    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(80, 80, 85));
}

void RedlineSSAudioProcessorEditor::configureLabel(juce::Label &label, const juce::String &text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::FontOptions(15.0f, juce::Font::bold));
}
