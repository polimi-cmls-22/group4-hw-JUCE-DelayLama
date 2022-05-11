/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"


namespace processor_consts
{
    //number of partials to generate for additive synthesis
    static constexpr int NUM_PARTIALS = 4;
    //Define the number of polyphonies (max number of MIDI notes that can be played at the same time)
    static constexpr int NUM_VOICES = 10;
}

//==============================================================================
/**
*/
class FractalSynthesisAudioProcessor : public juce::AudioProcessor, juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    FractalSynthesisAudioProcessor();
    ~FractalSynthesisAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;


    void parameterChanged(const juce::String& parameterID, float newValue) override;






    juce::AudioProcessorValueTreeState apvts;

  
    
    juce::OwnedArray<juce::AudioVisualiserComponent> waveVisualisers;
 
private:


    //Synth variables



    std::vector<juce::Synthesiser*> synths;

    juce::Synthesiser* synth;

    std::vector<std::complex<double>> fractalPoints = { 0, 0, 0, 0 }; //to store the fractal points

    std::vector<double> lfoRates = { 3, 3, 3, 3 }; //Hz, default values
    std::vector<double> freqDetunes = { 1, 2, 3, 4 }; //default values

    juce::AudioProcessorValueTreeState::ParameterLayout createParams();


    //static utility functions for fractal computation
    static std::complex<double> mandelbrot(std::complex<double> z, std::complex<double> c);

    static std::complex<double> burningShip(std::complex<double> z, std::complex<double> c);
    
    static std::complex<double> tricorn(std::complex<double> z, std::complex<double> c);

    std::complex<double>(*currentFractal)(std::complex<double> z, std::complex<double> c) = mandelbrot; //default fractal

    void generateFractalSuccession(std::complex<double> c);

    void generateLFORates(std::vector<std::complex<double>> fractalSuccession);

    void generateFreqDetunes(std::vector<std::complex<double>> fractalSuccession);

    void updateADSR(int partialIndex, SynthVoice* voice);


    bool updatedFractal = true; //defaults to true to start up the first computation

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalSynthesisAudioProcessor)
};
