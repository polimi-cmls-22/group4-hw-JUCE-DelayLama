/*
  ==============================================================================

    InputPlane.h
    Created: 20 Apr 2022 10:57:42pm
    Author:  Ricky

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class InputPlane : public juce::Component, juce::Slider::Listener
{
public:
    InputPlane(juce::Slider& sliderX, juce::Slider& sliderY);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;

    void sliderValueChanged(juce::Slider* slider);

private:


    //Bounds for the input plane (actual "transformed" coordinates)
    //(Assuming a square so the bounds are the same for x and y)
    const float inputMax = 1;
    const float inputMin = -1;
    const float r = inputMax - inputMin;

    juce::Slider& sliderX;
    juce::Slider& sliderY;

};
