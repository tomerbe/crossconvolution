/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class FftshellAudioProcessorEditor  : public AudioProcessorEditor,
    public Slider::Listener
{
public:
    FftshellAudioProcessorEditor (FftshellAudioProcessor&);
    ~FftshellAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void sliderValueChanged(Slider* sliderThatWasMoved) override;

    Slider oneSlider;
    Slider twoSlider;
    Slider threeSlider;
    Slider fourSlider;
    Slider fiveSlider;
    ToggleButton convolveButton;
    
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> oneSliderAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> twoSliderAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> threeSliderAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> fourSliderAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> fiveSliderAttachment;
    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> convolveButtonAttachment;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FftshellAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FftshellAudioProcessorEditor)
};
