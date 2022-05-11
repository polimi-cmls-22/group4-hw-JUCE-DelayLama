/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "InputPlane.h"

//==============================================================================
FractalSynthesisAudioProcessorEditor::FractalSynthesisAudioProcessorEditor (FractalSynthesisAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), inputPlaneComponent(initialPointXSlider, initialPointYSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.



    fractalFunctionComboBox.addItem("Mandelbrot Set", 1);
    fractalFunctionComboBox.addItem("Burning Ship Set", 2);
    fractalFunctionComboBox.addItem("Tricorn", 3);


    xLabel.setText("X", juce::dontSendNotification);
    yLabel.setText("Y",juce::dontSendNotification);
    inputPlaneLabel.setText("Input plane", juce::dontSendNotification);

    xLabel.setJustificationType(juce::Justification::centred);
    xLabel.attachToComponent(&initialPointXSlider, true);

    yLabel.setJustificationType(juce::Justification::centred);
    yLabel.attachToComponent(&initialPointYSlider, true);

    inputPlaneLabel.setJustificationType(juce::Justification::centred);
    inputPlaneLabel.attachToComponent(&inputPlaneComponent, false);



    for (size_t i = 0; i < processor_consts::NUM_PARTIALS; i++)
    {

        attackSliders.add(new juce::Slider());
        decaySliders.add(new juce::Slider());
        sustainSliders.add(new juce::Slider());
        releaseSliders.add(new juce::Slider());
        waveTypeComboBoxes.add(new juce::ComboBox());

        attackLabels.add(new juce::Label());
        decayLabels.add(new juce::Label());
        sustainLabels.add(new juce::Label());
        releaseLabels.add(new juce::Label());
        waveTypeLabels.add(new juce::Label());
        

        setSliderStyle(attackSliders[i]);
        setSliderStyle(decaySliders[i]);
        setSliderStyle(sustainSliders[i]);
        setSliderStyle(releaseSliders[i]);


        attackLabels[i]->setText("Attack", juce::dontSendNotification);
        attackLabels[i]->setJustificationType(juce::Justification::centred);
        attackLabels[i]->attachToComponent(attackSliders[i], false);

        decayLabels[i]->setText("Decay", juce::dontSendNotification);
        decayLabels[i]->setJustificationType(juce::Justification::centred);
        decayLabels[i]->attachToComponent(decaySliders[i], false);


        sustainLabels[i]->setText("Sustain", juce::dontSendNotification);
        sustainLabels[i]->setJustificationType(juce::Justification::centred);
        sustainLabels[i]->attachToComponent(sustainSliders[i], false);


        releaseLabels[i]->setText("Release", juce::dontSendNotification);
        releaseLabels[i]->setJustificationType(juce::Justification::centred);
        releaseLabels[i]->attachToComponent(releaseSliders[i], false);

 
        waveTypeLabels[i]->attachToComponent(waveTypeComboBoxes[i], false);
        waveTypeLabels[i]->setJustificationType(juce::Justification::centred);
        waveTypeLabels[i]->setText("Wave type", juce::dontSendNotification);
        waveTypeComboBoxes[i]->addItem("Sine", 1);
        waveTypeComboBoxes[i]->addItem("Saw", 2);
        waveTypeComboBoxes[i]->addItem("Square", 3);

        attackAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "ATTACK" + std::to_string(i), *attackSliders[i]);

        sustainAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SUSTAIN" + std::to_string(i), *sustainSliders[i]);

        decayAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "DECAY" + std::to_string(i), *decaySliders[i]);

        releaseAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "RELEASE" + std::to_string(i), *releaseSliders[i]);

        waveTypeAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "WAVE_TYPE" + std::to_string(i), *waveTypeComboBoxes[i]);

    }


    fractalComboBoxAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "FRACTAL_FUNCTION", fractalFunctionComboBox);


    sliderInitialPointXAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "INITIAL_POINT_X", initialPointXSlider);


    sliderInitialPointYAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "INITIAL_POINT_Y", initialPointYSlider);
    

    mandelbrotImage = juce::ImageCache::getFromMemory(BinaryData::Mandelbrot2_png, BinaryData::Mandelbrot2_pngSize);
    burningShipImage = juce::ImageCache::getFromMemory(BinaryData::BurningShip2_png, BinaryData::BurningShip2_pngSize);
    tricornImage = juce::ImageCache::getFromMemory(BinaryData::Tricorn2_png, BinaryData::Tricorn2_pngSize);
    

    currentImage = mandelbrotImage;

    fractalFunctionComboBox.addListener(this);
   
    addAndMakeVisible(fractalFunctionComboBox);

    addAndMakeVisible(inputPlaneComponent);

    addAndMakeVisible(initialPointXSlider);

    addAndMakeVisible(initialPointYSlider);


    for (size_t i = 0; i < processor_consts::NUM_PARTIALS; i++)
    {
        addAndMakeVisible(attackSliders[i]);

        addAndMakeVisible(sustainSliders[i]);

        addAndMakeVisible(decaySliders[i]);

        addAndMakeVisible(releaseSliders[i]);

        addAndMakeVisible(waveTypeComboBoxes[i]);

       addAndMakeVisible(audioProcessor.waveVisualisers[i]);

    }


    setSize(700, 700);

}

FractalSynthesisAudioProcessorEditor::~FractalSynthesisAudioProcessorEditor()
{
}

//==============================================================================
void FractalSynthesisAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setOpacity(0.4);
    //background image
    g.drawImageWithin(currentImage, 0, 0, getLocalBounds().getWidth(), getLocalBounds().getHeight(),
        juce::RectanglePlacement::fillDestination);

 

    g.setColour(juce::Colours::floralwhite);
    g.setOpacity(0.8);
    g.drawRoundedRectangle(osc1Area.toFloat().reduced(15), 5.0f, 4.0f);
    g.drawText("OSC 1", osc1Area.toFloat().removeFromTop(10), juce::Justification::left);
    

    g.drawRoundedRectangle(osc2Area.toFloat().reduced(15), 5.0f, 4.0f);
    g.drawText("OSC 2", osc2Area.toFloat().removeFromTop(10), juce::Justification::left);

    g.drawRoundedRectangle(osc3Area.toFloat().reduced(15), 5.0f, 4.0f);
    g.drawText("OSC 3", osc3Area.toFloat().removeFromTop(10), juce::Justification::left);

    g.drawRoundedRectangle(osc4Area.toFloat().reduced(15), 5.0f, 4.0f);
    g.drawText("OSC 4", osc4Area.toFloat().removeFromTop(10), juce::Justification::left);


    g.drawRoundedRectangle(fractalArea.toFloat(), 5.0f, 4.0f);
    g.drawRoundedRectangle(inputPlaneArea.toFloat(), 5.0f, 3.0f);


    g.setColour(juce::Colours::darkorange);
    for (size_t i = 0; i < processor_consts::NUM_PARTIALS; i++)
    {
        g.drawRoundedRectangle(audioProcessor.waveVisualisers[i]->getBounds().expanded(3).toFloat(), 5.0f, 2.0f);
    }


}

void FractalSynthesisAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds().reduced(20);

    auto oscArea = bounds.removeFromTop(bounds.getHeight() / 1.5);
    auto upperOscArea = oscArea.removeFromTop(oscArea.getHeight() / 2);
    auto lowerOscArea = oscArea;
    osc1Area = upperOscArea.removeFromLeft(upperOscArea.getWidth() / 2);
    osc2Area = upperOscArea;
    osc3Area = lowerOscArea.removeFromLeft(lowerOscArea.getWidth() / 2);
    osc4Area = lowerOscArea;
    fractalArea = bounds;


    buildOscSubArea(0, osc1Area);
    buildOscSubArea(1, osc2Area);
    buildOscSubArea(2, osc3Area);
    buildOscSubArea(3, osc4Area);

    buildFractalArea(fractalArea);

}

void FractalSynthesisAudioProcessorEditor::comboBoxChanged(juce::ComboBox* combo){
    

    if (combo->getSelectedId() == 1)
        currentImage = mandelbrotImage;
    else if (combo->getSelectedId() == 2)
        currentImage = burningShipImage;
    else if(combo->getSelectedId() == 3)
        currentImage = tricornImage;


    repaint();

}


void FractalSynthesisAudioProcessorEditor::setSliderStyle(juce::Slider* slider)
{

    slider->setSliderStyle(juce::Slider::Rotary);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    slider->setPopupDisplayEnabled(true, true, this);
    slider->setColour(juce::Slider::ColourIds::rotarySliderFillColourId
        , juce::Colours::ghostwhite);
    slider->setTextValueSuffix(" seconds");

}

void FractalSynthesisAudioProcessorEditor::buildOscSubArea(int index, juce::Rectangle<int> bounds)
{

    auto tempBounds = juce::Rectangle<int>(bounds); //create a copy so we don't modify the variable (needed by the paint method to draw the rectangle)
    tempBounds = tempBounds.reduced(35);
    auto oscADSRArea = tempBounds.removeFromLeft(tempBounds.getWidth() / 2);
    auto oscWaveTypeArea = tempBounds.removeFromTop(tempBounds.getHeight() / 2.5);
    auto oscWaveVisualizerArea = tempBounds;
    auto oscADArea = oscADSRArea.removeFromTop(oscADSRArea.getHeight() / 2);
    auto oscSRArea = oscADSRArea;

    attackSliders[index]->setBounds(oscADArea.removeFromLeft(oscADArea.getWidth() / 2).reduced(3));
    decaySliders[index]->setBounds(oscADArea.reduced(3));
    sustainSliders[index]->setBounds(oscSRArea.removeFromLeft(oscSRArea.getWidth() / 2).reduced(3));
    releaseSliders[index]->setBounds(oscSRArea.reduced(3));

    waveTypeComboBoxes[index]->setBounds(oscWaveTypeArea.withHeight(oscWaveTypeArea.getHeight() * 0.95));
    audioProcessor.waveVisualisers[index]->setBounds(oscWaveVisualizerArea.withHeight(oscWaveVisualizerArea.getHeight()*0.95).reduced(3));
    
}


void FractalSynthesisAudioProcessorEditor::buildFractalArea(juce::Rectangle<int> fractalArea)
{
    auto tempBounds = juce::Rectangle<int>(fractalArea).reduced(10);

    inputPlaneArea = tempBounds.removeFromLeft(tempBounds.getWidth() * 0.33).reduced(5);
    inputPlaneComponent.setBounds(inputPlaneArea.reduced(3));

    auto comboBoxArea = tempBounds.removeFromTop(tempBounds.getHeight() * 0.33).reduced(5);
    fractalFunctionComboBox.setBounds(comboBoxArea);

    auto initialPointXSliderArea = (tempBounds.removeFromTop(tempBounds.getHeight() * 0.5).reduced(5));
    initialPointXSlider.setBounds(initialPointXSliderArea.reduced(5));

    auto initialPointYSliderArea = (tempBounds.reduced(5));
    initialPointYSlider.setBounds(initialPointYSliderArea.reduced(5));

}



    
