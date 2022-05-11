/*
  ==============================================================================

    InputPlane.cpp
    Created: 20 Apr 2022 10:58:06pm
    Author:  Ricky

  ==============================================================================
*/

#include "InputPlane.h"
#include <JuceHeader.h>

InputPlane::InputPlane(juce::Slider& sliderX, juce::Slider& sliderY) : sliderX(sliderX), sliderY(sliderY)
{

    sliderX.addListener(this);
    sliderY.addListener(this);

}

void InputPlane::paint(juce::Graphics& g)
{


    auto area = getLocalBounds();
    int width = area.getWidth();
    int height = area.getHeight();

    g.fillAll(juce::Colour::fromRGB(0, 0, 0));

    g.setColour(juce::Colours::aliceblue);

    //Inverse mapping
    float x = (sliderX.getValue() - inputMin) * width / r;
    float y = height - (sliderY.getValue() - inputMin) * height / r;

    g.drawEllipse(x, y, 5, 5, 1);

}

void InputPlane::resized()
{
}

void InputPlane::mouseDown(const juce::MouseEvent& event)
{

    auto area = getLocalBounds();

    int width = area.getWidth();
    int height = area.getHeight();

    float x = (float)event.getMouseDownX() * r / width + inputMin;

    //First flip the y (since the coordinate system used by the graphics component has the y reversed
    //compared to the usual cartesian coordinate system)
    float y = height - event.getMouseDownY();

    //Then map range
    y = y * r / height + inputMin;


    sliderX.setValue(x);
    sliderY.setValue(y);

    repaint();

}

void InputPlane::sliderValueChanged(juce::Slider* slider)
{

    if (sliderX.getValue() == 0 && sliderY.getValue() == 0)
    {
        //Set to 0.01 to avoid errors due to zero frequencies
        slider->setValue(0.01);
    }
    repaint();
}

