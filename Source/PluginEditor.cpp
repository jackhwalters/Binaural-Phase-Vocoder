/*
  ==============================================================================

        DAFX BINAURAL PHASE VOCODER
        v1.0
        Jack Walters

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
/*
  * @brief Constructor which initialises variables and instantiates all JUCE sliders and buttons
  * @param The plugin's audio processor class
*/
DafxBinauralPhaseVocoderAudioProcessorEditor::DafxBinauralPhaseVocoderAudioProcessorEditor (DafxBinauralPhaseVocoderAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , processor (p)
    , elevationSlider_(0)
    , gainSlider_(0)
    , bgColour_(255,0,0)
    , fgColour_(0,0,255)
{
    // ID used to group buttons together to ensure they are mutually exclusive (i.e. only one is on at a time)
    enum RadioButtonIds
    {
        buttonRadioID = 1001
    };
    
    //GUI object instantiation
    // Instantiate bypass button
    addAndMakeVisible (bypassButton_ = new TextButton (L"Bypass button"));
    bypassButton_->setButtonText (L"Bypass Plugin");
    bypassButton_->addListener (this);
    bypassButton_->setColour (TextButton::buttonColourId, Colour (0xff615a5a));
    bypassButton_->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    bypassButton_->setClickingTogglesState(true);
    bypassButton_->setRadioGroupId(buttonRadioID);
    bypassButton_->setColour(0x1000101, Colours::red);
    
    // Instantiate vocoder pass-through button
    addAndMakeVisible (passThroughButton_ = new TextButton (L"Pass-through button"));
    passThroughButton_->setButtonText (L"Pass-through \nVocoder");
    passThroughButton_->addListener (this);
    passThroughButton_->setToggleState(true, dontSendNotification);
    passThroughButton_->setColour (TextButton::buttonColourId, Colour (0xff615a5a));
    passThroughButton_->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    passThroughButton_->setClickingTogglesState(true);
    passThroughButton_->setRadioGroupId(buttonRadioID);
    
    // Instantiate vocoder robotisation button
    addAndMakeVisible (robotisationButton_ = new TextButton (L"Robotisation button"));
    robotisationButton_->setButtonText (L"Robotisation");
    robotisationButton_->addListener (this);
    robotisationButton_->setColour (TextButton::buttonColourId, Colour (0xff615a5a));
    robotisationButton_->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    robotisationButton_->setClickingTogglesState(true);
    robotisationButton_->setRadioGroupId(buttonRadioID);
    
    // Instantiate vocoder whisperisation button
    addAndMakeVisible (whisperisationButton_ = new TextButton (L"Whisperisation button"));
    whisperisationButton_->setButtonText (L"Whisperisation");
    whisperisationButton_->addListener (this);
    whisperisationButton_->setColour (TextButton::buttonColourId, Colour (0xff615a5a));
    whisperisationButton_->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    whisperisationButton_->setClickingTogglesState(true);
    whisperisationButton_->setRadioGroupId(buttonRadioID);
    
    // Instantiate slider for adjusting virtual sound source elevation relative to listener
    addAndMakeVisible (elevationSlider_ = new Slider (L"Elevation slider"));
    elevationSlider_->setRange (-90, 90, 0.1);
    elevationSlider_->setValue(0);
    elevationSlider_->setSliderStyle (Slider::LinearVertical);
    elevationSlider_->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    elevationSlider_->setPopupDisplayEnabled (false, false, this, 2000);
    elevationSlider_->addListener (this);
    
    // Instantiate slider for adjusting virtual sound source distance from listener
    addAndMakeVisible (distanceSlider_ = new Slider (L"Distance slider"));
    distanceSlider_->setRange (1, 20, 0.1);
    distanceSlider_->setValue(2.0);
    distanceSlider_->setSkewFactorFromMidPoint(7);    // log scale so closer distance can be adjusted more finely
    distanceSlider_->setSliderStyle (Slider::LinearHorizontal);
    distanceSlider_->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    distanceSlider_->setPopupDisplayEnabled (false, false, this, 2000);
    distanceSlider_->addListener (this);
    
    // Instantiate slider for overall gain
    addAndMakeVisible (gainSlider_ = new Slider (L"Gain slider"));
    gainSlider_->setRange (0, 3, 0.01);
    gainSlider_->setValue(1.0);
    gainSlider_->setSliderStyle (Slider::LinearBarVertical);
    gainSlider_->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    gainSlider_->setPopupDisplayEnabled (false, false, this, 2000);
    gainSlider_->addListener (this);
    
    // Load images (from C arrays in the "images" folder) through JUCE's image reading codec
    sourceImage_ = ImageFileFormat::loadFrom(source_icon_png, source_icon_png_size);
    headImage_ = ImageFileFormat::loadFrom(head_top_png, head_top_png_size);
    
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
    resizeLimits.setSizeLimits (150, 150, 850, 290);
    setSize (880, 720);
    startTimer(50);
    
    // Initialise the virtual sound source's azimuth and elevation
    sourcePos_.azimuth = 0.0;
    sourcePos_.elevation = 0;
}

DafxBinauralPhaseVocoderAudioProcessorEditor::~DafxBinauralPhaseVocoderAudioProcessorEditor()
{
    // Buttons
    deleteAndZero (bypassButton_);
    deleteAndZero (passThroughButton_);
    deleteAndZero (robotisationButton_);
    deleteAndZero (whisperisationButton_);
    
    // Sliders
    deleteAndZero (elevationSlider_);
    deleteAndZero (distanceSlider_);
    deleteAndZero (gainSlider_);
}

//==============================================================================
/*
  * @brief Paints the background of the component window and instantiates text boxes
  * @param The JUCE graphics class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::paint (Graphics& g)
{
    // Make background colour grey
    g.fillAll(Colours::grey);

    // Draw a fully opaque black circle
    g.setOpacity(1.0f);
    g.setColour(Colours::black);
    auto w = getWidth();
    auto h = getHeight();
    g.fillEllipse(0.01*w, 0.01*h, static_cast<float>(w*0.74), static_cast<float>(h*0.9));

    // Call grid line and blue sound source drawing functions
    drawGridLines(g);
    drawSource(g);

    // Draw a fully opaque head from C array in the centre of the circle
    g.setOpacity(1.f);
    g.drawImageWithin(headImage_, 0.01*w, 0.01*h, w*0.731, h*0.9, RectanglePlacement::centred | RectanglePlacement::doNotResize);
    
    // Display distance measurement in text box, appended with an 'm' symbol
    g.setColour (Colours::white);
    g.setFont (15.5f);
    String distanceString (processor.distance, 2);
    distanceString += "m";
    g.drawFittedText(String(distanceString), 380, 670, 50, 50, Justification::centred, 1);
    
    // Display elevation measurement in text box, appended with a ° symbol (represented by the unicode character "\u00B0")
    g.setColour (Colours::white);
    g.setFont (15.5f);
    String elevationString (processor.elevation);
    String elevationStringDeg = elevationString + "\u00B0";
    if (processor.elevation > 0)    // If the elevation is above 0, prepend the reading with a + symbol (represented by the unicode character "\u002B")
    {
        String positiveElevationStringDeg = "\u002B" + elevationStringDeg;
        g.drawFittedText(String(positiveElevationStringDeg), 480, 670, 50, 50, Justification::centred, 1);
    }
    else if (processor.elevation <= 0)
    {
        g.drawFittedText(String(elevationStringDeg), 480, 670, 50, 50, Justification::centred, 1);
    }
    
    // Display azimuth measurement in degrees rather than radians, and append a ° symbol (represented by the unicode character "\u00B0")
    g.setColour (Colours::white);
    g.setFont (15.5f);
    String azimuthString (rad2deg(processor.azimuth),2);
    String azimuthStringDeg = azimuthString + "\u00B0";
    if (processor.azimuth > 0)    // If the azimuth is above 0, prepend the reading with a + symbol (represented by the unicode character "\u002B")
    {
        String positiveAzimuthStringDeg = "\u002B" + azimuthStringDeg;
        g.drawFittedText(String(positiveAzimuthStringDeg), 570, 670, 70, 50, Justification::centred, 1);
    }
    else
    {
        g.drawFittedText(String(azimuthStringDeg), 570, 670, 70, 50, Justification::centred, 1);
    }
    
    //The following code segments are for instantiating text boxes labels on the GUI
    g.setColour (Colours::white);
    g.setFont (14.8f);
    String gainString(processor.audioGain,2);
    g.drawFittedText(String(gainString), 795, 136, 50, 50, Justification::centred, 1);

    g.setColour (Colours::black);
    g.setFont (17.0f);
    g.drawFittedText(String("Distance"), 380, 650, 50, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (16.5f);
    g.drawFittedText(String("Sounce Source Distance"), 18, 645, 150, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (17.0f);
    g.drawFittedText(String("Elevation"), 480, 650, 50, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (16.5f);
    g.drawFittedText(String("Sound Source Elevation"), 515, -4, 150, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (16.0f);
    g.drawFittedText(String("Azimuth"), 580, 650, 50, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (16.0f);
    g.drawFittedText(String("Gain"), 743, 136, 80, 50, Justification::centred, 1);
    
    g.setColour (Colours::black);
    g.setFont (16.0f);
    g.drawFittedText(String("v1.0\nJack Walters"), -18, 3, 150, 50, Justification::centred, 1);
}

/*
  * @brief Draw grid lines to denote the four quadrants in the listener's periphery
  * @param The JUCE graphics class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::drawGridLines(Graphics &g)
{
    g.setColour(Colours::white);
    g.setOpacity(0.5f);
    auto w = static_cast<float>(getWidth());
    auto h = static_cast<float>(getHeight());
    auto lineHorizontal = Line<float>(0.01*w, h*0.46, w*0.75, h*0.46);
    auto lineVertical = Line<float>(w*0.375, h*0.01, w*0.375, h*0.91);
    float dashes[] = {3, 2};
    g.drawDashedLine(lineHorizontal, dashes, 2);
    g.drawDashedLine(lineVertical, dashes, 2);
}

/*
  * @brief Draw the blue sound source in the GUI diagram
  * @param The JUCE graphics class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::drawSource(Graphics& g)
{
    auto w = static_cast<float>(getWidth());
    auto h = static_cast<float>(getHeight());
    auto radius = w * (0.39 * (0.5 + ((1.0 - 0.5) / (20 - 1)) * (processor.distance - 1))) - 30;    // Map the distance of the virtual sound source (in meters) to the distance the blue sound soure is from the head in the diagram
    
    // Assign cursor x-axis and y-axis values to be in accordance with the radius
    auto x = radius * std::sin(sourcePos_.azimuth) * std::cos(sourcePos_.elevation);
    auto y = radius * std::cos(sourcePos_.azimuth) * std::cos(sourcePos_.elevation);
    // Reassign x and y so that their readings are centred around the centre of the circle
    x = (w*0.37) + (x*0.9);
    y = (h*0.45) - (y*0.9);
    auto color = fgColour_;

    // Draw a transparent black cirle the same size as the first, however not as opaque, to allow the source image below to permeate through
    ColourGradient grad(color, x, y, Colours::transparentBlack, x + w * 0.25f, y + h * 0.25f, true);
    g.setGradientFill(grad);
    g.fillEllipse(0.01 * w, 0.01 * h, static_cast<float>(w*0.74), static_cast<float>(h*0.9));
    auto scaleFactor = 0.5 * (std::sin(sourcePos_.elevation) * 0.4f + 0.2);    //Assign scale factor for source image
    
    // Draw the source within a rectangle, based off the source width, x and y cursor reading and scale factor
    auto sw = sourceImage_.getWidth();
    auto sh = sourceImage_.getHeight();
    g.drawImageWithin(sourceImage_,
        static_cast<int>(x - sw * scaleFactor),
        static_cast<int>(y - sh * scaleFactor),
        static_cast<int>(sw * scaleFactor),
        static_cast<int>(sh * scaleFactor),
        RectanglePlacement::centred,
        true);
    
    //Call function to send azimuth value to DafxBinauralPhaseVocoderAudioProcessor
    sourceLocationChanged(sourcePos_.azimuth);
}

/*
  * @brief Attain x and y readings from when the user has clicked the mouse and is dragging it
  * @param The JUCE MouseEvent class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::mouseDrag(const MouseEvent& event)
{
    auto pos = event.getPosition();
    
    // Reassign x and y so that their readings are centred around the centre of the circle
    auto x = pos.x - (getWidth() * 0.375f);
    auto y = pos.y - (getHeight() * 0.46f);
    
    // Deduce azimth angle based off the arc tangent of x/-y
    sourcePos_.azimuth = std::atan2(x, -y);
    Component::repaint();
}

/*
  * @brief Set GUI positions of buttons and sliders
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::resized()
{
    // Buttons
    bypassButton_->setBounds(690,655,180,50);
    passThroughButton_->setBounds(715,210,135,100);
    robotisationButton_->setBounds(715,360,135,100);
    whisperisationButton_->setBounds(715,510,135,100);
    
    // Sliders
    elevationSlider_->setBounds(630,6,100,650);
    distanceSlider_->setBounds(10, 680, 320, 20);
    gainSlider_->setBounds(717,20,40,150);
}

/*
  * @brief Deduce what button was pressed and send corresponding value to the DafxBinauralPhaseVocoderAudioProcessor class
  * @param The JUCE button class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::buttonClicked(Button* button)
{
    if (button == bypassButton_)
    {
        processor.bypass = bypassButton_->getToggleState();
    }
    else if (button == passThroughButton_)
    {
        processor.passthrough = passThroughButton_->getToggleState();
    }
    else if (button == robotisationButton_)
    {
        processor.robotisation = robotisationButton_->getToggleState();
    }
    else if (button == whisperisationButton_)
    {
        processor.whisperisation = whisperisationButton_->getToggleState();
    }

    Component::repaint();
}

/*
  * @brief Deduce what slider was dragged and send corresponding value to the DafxBinauralPhaseVocoderAudioProcessor class
  * @param The JUCE Slider class
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::sliderValueChanged(Slider* sliderThatWasMoved)
{
    if (sliderThatWasMoved == gainSlider_)
    {
        processor.audioGain = gainSlider_->getValue();
    }
    else if (sliderThatWasMoved == elevationSlider_)
    {
        processor.elevation = elevationSlider_->getValue();
    }
    else if (sliderThatWasMoved == distanceSlider_)
    {
        processor.distance = distanceSlider_->getValue();
    }
}

/*
  * @brief Read azimuth values from GUI and send value to the DafxBinauralPhaseVocoderAudioProcessor class
  * @param
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::sourceLocationChanged(float azimuth)
{
    processor.azimuth = azimuth;
}

/*
  * @brief Continually read for changes in button values every 50ms (value initiated in constructor)
*/
void DafxBinauralPhaseVocoderAudioProcessorEditor::timerCallback()
{
    // Display the ON/OFF button in its correct state
    if (bypassButton_->getToggleState())
    {
        bypassButton_->setToggleState(true, dontSendNotification);
    }
    else if (bypassButton_->getToggleState() == false)
    {
        bypassButton_->setToggleState(false, dontSendNotification);
    }
    else if (passThroughButton_->getToggleState())
    {
        passThroughButton_->setToggleState(true, dontSendNotification);
    }
    else if (passThroughButton_->getToggleState() == false)
    {
        passThroughButton_->setToggleState(false, dontSendNotification);
    }
    else if (robotisationButton_->getToggleState())
    {
        robotisationButton_->setToggleState(true, dontSendNotification);
    }
    else if (robotisationButton_->getToggleStateValue() == false)
    {
        robotisationButton_->setToggleState(false, dontSendNotification);
    }
    else if (whisperisationButton_->getToggleState())
    {
        whisperisationButton_->setToggleState(true, dontSendNotification);
    }
    else if (whisperisationButton_->getToggleStateValue() == false)
    {
        whisperisationButton_->setToggleState(false, dontSendNotification);
    }
    
    Component::repaint();
}
