/*
  ==============================================================================

    SynthSound.h
    Created: 10 Apr 2022 8:10:53pm
    Author:  Ricky

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int midiNoteNumber) override { return true;  }
    bool appliesToChannel(int midiChannel) override { return true;  }
};