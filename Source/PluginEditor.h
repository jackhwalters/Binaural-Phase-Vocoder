/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Util.h"
#include "images/head_top.h"
#include "images/source_icon.h"
#include <cmath>

//==============================================================================
/**
*/
class DafxBinauralPhaseVocoderAudioProcessorEditor  : public AudioProcessorEditor
    , public Timer
    , public Button::Listener
    , public Slider::Listener
{
public:
    DafxBinauralPhaseVocoderAudioProcessorEditor (DafxBinauralPhaseVocoderAudioProcessor&);
    ~DafxBinauralPhaseVocoderAudioProcessorEditor();

    //==============================================================================
    // Static GUI elements
    void paint(Graphics& g);
    void drawFittedText();
    void drawGridLines(Graphics& g);
    void resized();
    
    //Timer
    void timerCallback();
    
    //Buttons
    void buttonClicked (Button* buttonThatWasClicked);
    
    //Sliders
    void sliderValueChanged (Slider* sliderThatWasMoved);
    
    //Non-JUCE GUI interactable objects
    void sourceLocationChanged (float azimuth);
    
    //Moveable source
    void mouseDrag(const MouseEvent& event);
    void drawSource(Graphics& g);


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DafxBinauralPhaseVocoderAudioProcessor& processor;
    
    ScopedPointer<ResizableCornerComponent> resizer;
    ComponentBoundsConstrainer resizeLimits;
        
    //Buttons
    TextButton* bypassButton_;
    TextButton* passThroughButton_;
    TextButton* robotisationButton_;
    TextButton* whisperisationButton_;
    
    //Sliders
    Slider* elevationSlider_;
    Slider* distanceSlider_;
    Slider* gainSlider_;
    
    //Static GUI
    Image headImage_;
    Image sourceImage_;
    Colour bgColour_;
    Colour fgColour_;
    
    //Draw blue image sound source based on dragged cursor location
    Point3DoublePolar<float> sourcePos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DafxBinauralPhaseVocoderAudioProcessorEditor)
};
