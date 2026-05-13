#include "PluginEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

void SwitchLookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &button, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(4.0f);

    auto textArea = bounds.removeFromBottom(20.0f);
    auto switchArea = bounds.withSizeKeepingCentre(54.0f, 26.0f);

    const auto isOn = button.getToggleState();

    g.setColour(isOn ? juce::Colours::orange : juce::Colours::darkgrey);
    g.fillRoundedRectangle(switchArea, 13.0f);

    auto knob = switchArea.reduced(3.0f);
    knob.setWidth(20.0f);

    if (isOn)
        knob.setX(switchArea.getRight() - 23.0f);

    g.setColour(juce::Colours::white);
    g.fillEllipse(knob);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(button.getButtonText(), textArea, juce::Justification::centred);
}

RedlineSSAudioProcessorEditor::RedlineSSAudioProcessorEditor(RedlineSSAudioProcessor &p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1024, 260);
    setResizable(true, true);
    setResizeLimits(600, 200, 2000, 800);

    inputHighPassAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "inputHighPassHz", inputHighPassSlider);

    dirtyToggle.setButtonText("Dirty");
    dirtyToggle.setLookAndFeel(&switchLookAndFeel);
    addAndMakeVisible(dirtyToggle);

    dirtyAttachment = std::make_unique<ButtonAttachment>(processor.apvts, "dirty", dirtyToggle);

    gain1Attachment = std::make_unique<SliderAttachment>(processor.apvts, "gain1", gain1Slider);

    bias1Attachment = std::make_unique<SliderAttachment>(processor.apvts, "bias1", bias1Slider);

    interstageHighPassAttachment = std::make_unique<SliderAttachment>(
        processor.apvts, "interstageHighPassHz", interstageHighPassSlider);

    gain2Attachment = std::make_unique<SliderAttachment>(processor.apvts, "gain2", gain2Slider);

    interstageHighPassAttachment2 = std::make_unique<SliderAttachment>(
        processor.apvts, "interstageHighPassHz2", interstageHighPassSlider2);

    gain3Attachment = std::make_unique<SliderAttachment>(processor.apvts, "gain3", gain3Slider);

    bassAttachment = std::make_unique<SliderAttachment>(processor.apvts, "bassDb", bassSlider);

    lowerMidAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "lowerMidDb", lowerMidSlider);

    upperMidAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "upperMidDb", upperMidSlider);

    trebleAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "trebleDb", trebleSlider);

    fizzLowPassAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "fizzLowPassHz", fizzLowPassSlider);

    masterLevelAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "masterLevel", masterLevelSlider);

    resonanceAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "resonanceDb", resonanceSlider);

    thresholdAttachment =
        std::make_unique<SliderAttachment>(processor.apvts, "thresholdDb", thresholdSlider);

    auto setupKnob = [this](juce::Slider &slider, juce::Label &label, const juce::String &text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupKnob(inputHighPassSlider, tightLabel, "Tight");
    setupKnob(gain1Slider, gain1Label, "Gain 1");
    setupKnob(bias1Slider, bias1Label, "Bias 1");
    setupKnob(interstageHighPassSlider, cutLabel, "Cut");
    setupKnob(gain2Slider, gain2Label, "Gain 2");
    setupKnob(interstageHighPassSlider2, cut2Label, "Cut 2");
    setupKnob(gain3Slider, gain3Label, "Gain 3");
    setupKnob(bassSlider, bassLabel, "Bass");
    setupKnob(lowerMidSlider, lowerMidLabel, "Low Mid");
    setupKnob(upperMidSlider, upperMidLabel, "Upper Mid");
    setupKnob(trebleSlider, trebleLabel, "Treble");
    setupKnob(fizzLowPassSlider, fizzLabel, "Fizz");
    setupKnob(masterLevelSlider, masterLevelLabel, "Level");
    setupKnob(resonanceSlider, resonanceLabel, "Resonance");
    setupKnob(thresholdSlider, thresholdLabel, "Threshold");
}

void RedlineSSAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour::fromRGB(20, 20, 22));

    g.setColour(juce::Colours::white);
    g.setFont(22.0f);

    g.drawFittedText(
        "Redline SS", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);

    g.setFont(14.0f);
}

void RedlineSSAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(50);

    const auto knobSize = 90;
    const auto gap = 10;

    auto x = area.getX();
    auto y = area.getY();

    dirtyToggle.setBounds(x, y, knobSize, knobSize);
    x += knobSize + gap;

    for (auto *slider :
         {&inputHighPassSlider,
          &gain1Slider,
          &bias1Slider,
          &interstageHighPassSlider,
          &gain2Slider,
          &interstageHighPassSlider2,
          &gain3Slider,
          &bassSlider,
          &lowerMidSlider,
          &upperMidSlider,
          &trebleSlider,
          &fizzLowPassSlider,
          &masterLevelSlider,
          &resonanceSlider,
          &thresholdSlider}) {

        if (x + knobSize > area.getRight()) {
            x = area.getX();
            y += knobSize + 40;
        }
        slider->setBounds(x, y, knobSize, knobSize);
        x += knobSize + gap;
    }
}

RedlineSSAudioProcessorEditor::~RedlineSSAudioProcessorEditor()
{
    dirtyToggle.setLookAndFeel(nullptr);
}
