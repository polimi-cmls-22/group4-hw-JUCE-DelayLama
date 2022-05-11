/*
  ==============================================================================

    SynthVoice.h
    Created: 10 Apr 2022 8:10:44pm
    Author:  Ricky

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SynthSound.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:

    SynthVoice(int numPartials);
    

    bool canPlaySound(juce::SynthesiserSound*) override;

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;

    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannelsNumber);

    void setGain(float gainValue);

    void setPan(float panValue);

    void setFreqDetunes(const std::vector<double>& freqDetunes);

    void setLFORates(const std::vector<double>& lfoRates);

    void setLFODepths(const std::vector<double>& lfoDepths);

    void updateADSR(int i, const float attack, const float decay, const float sustain, const float release);


    enum chainElements
    {
        oscIndex,
        gainIndex,
        panIndex
    };

    void setWaveType(const int partialIndex, const int choice);

    void applyLFO(int i);
    
    //Public to be able to access it in the plugin processor
    juce::OwnedArray<juce::AudioBuffer<float>> synthBuffers; //Local buffers to temporarily store synth output (one for each partial)

private:

    enum waveTypes
    {
        SINE,
        SAW,
        SQUARE
    };

    int numPartials;

    std::vector< juce::dsp::ProcessorChain<juce::dsp::Oscillator<float>, juce::dsp::Gain<float>, juce::dsp::Panner<float>>> processorChains;

    std::vector<juce::ADSR> adsr;
    std::vector<juce::ADSR::Parameters> adsrParams;

    std::vector < juce::dsp::Oscillator<float>> lfos;


    std::vector<float> lfoRates;
    std::vector<float> lfoDepths;

    static constexpr size_t lfoUpdateRate = 100;
    
    size_t lfoUpdateCounter = lfoUpdateRate;

    std::vector<float> fixedGains;

    std::vector<float> detuneFactors;

    bool isPrepared = false;

    

};
