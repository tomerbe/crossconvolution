/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FftshellAudioProcessorEditor::FftshellAudioProcessorEditor (FftshellAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    
    oneSlider.setRange(-24.0, 24.0);
    oneSlider.setSliderStyle (Slider::LinearBarVertical);
    oneSlider.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(&oneSlider);
    oneSlider.addListener(this);
    oneSliderAttachment = new AudioProcessorValueTreeState::SliderAttachment (processor.parameters, "one", oneSlider);

    twoSlider.setRange(-24.0, 24.0);
    twoSlider.setSliderStyle (Slider::LinearBarVertical);
    twoSlider.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(&twoSlider);
    twoSlider.addListener(this);
    twoSliderAttachment = new AudioProcessorValueTreeState::SliderAttachment (processor.parameters, "two", twoSlider);
 
    threeSlider.setRange(-24.0, 24.0);
    threeSlider.setSliderStyle (Slider::LinearBarVertical);
    threeSlider.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(&threeSlider);
    threeSlider.addListener(this);
    threeSliderAttachment = new AudioProcessorValueTreeState::SliderAttachment (processor.parameters, "three", threeSlider);

    fourSlider.setRange(-24.0, 24.0);
    fourSlider.setSliderStyle (Slider::LinearBarVertical);
    fourSlider.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(&fourSlider);
    fourSlider.addListener(this);
    fourSliderAttachment = new AudioProcessorValueTreeState::SliderAttachment (processor.parameters, "four", fourSlider);

    fiveSlider.setRange(-24.0, 24.0);
    fiveSlider.setSliderStyle (Slider::LinearBarVertical);
    fiveSlider.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(&fiveSlider);
    fiveSlider.addListener(this);
    fiveSliderAttachment = new AudioProcessorValueTreeState::SliderAttachment (processor.parameters, "five", fiveSlider);

    addAndMakeVisible(&convolveButton);
    convolveButtonAttachment = new AudioProcessorValueTreeState::ButtonAttachment (processor.parameters, "convolve", convolveButton);
}

FftshellAudioProcessorEditor::~FftshellAudioProcessorEditor()
{
}

//==============================================================================
void FftshellAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Wello Horld!", getLocalBounds(), Justification::centredTop, 1);
}

void FftshellAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    oneSlider.setBounds (6, 30, 60, 300 - 36); // left, top, width, height
    twoSlider.setBounds (72, 30, 60, 300 - 36); // left, top, width, height
    threeSlider.setBounds (138, 30, 60, 300 - 36); // left, top, width, height
    fourSlider.setBounds (204, 30, 60, 300 - 36); // left, top, width, height
    fiveSlider.setBounds (270, 30, 60, 300 - 36); // left, top, width, height
    convolveButton.setBounds(347, 247, 36, 36);
}

void FftshellAudioProcessorEditor::sliderValueChanged(Slider* sliderThatWasMoved)
{
    //   if(sliderThatWasMoved == &gainSlider)
    //       processor.gain = gainSlider.getValue() * 10.0;
}
