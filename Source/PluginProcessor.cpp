/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthSound.h"
#include "SynthVoice.h"

//==============================================================================
FractalSynthesisAudioProcessor::FractalSynthesisAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts (*this, nullptr, "Parameters", FractalSynthesisAudioProcessor::createParams())//consutructors of the audio components
#endif
{

    //Create synth voices

    //add voicesNumber voices to the synth (allows voicesNumber MIDI notes to be played at the same time)
  
    synth = new juce::Synthesiser();
    synth->addSound(new SynthSound());
    
    for (size_t voice = 0; voice < processor_consts::NUM_VOICES; voice++)
    {
        synth->addVoice(new SynthVoice(processor_consts::NUM_PARTIALS));

    }

    //Create and init wave visualizers
    for (size_t i = 0; i < processor_consts::NUM_PARTIALS; i++)
    {
        auto waveVis = new juce::AudioVisualiserComponent(1);
        waveVis->setBufferSize(512);
        waveVis->setSamplesPerBlock(256);
        waveVis->setRepaintRate(30);
        waveVis->setColours(juce::Colours::black, juce::Colours::deepskyblue);
        waveVisualisers.add(waveVis);
    }

    //Add this as listener to the fractal params
    //(usually this is not needed when working with apvts and attachments,
    //but for performance reasons we don't want to recompute
    //the fractal succession all the time)
    apvts.addParameterListener("FRACTAL_FUNCTION", this);

    apvts.addParameterListener("INITIAL_POINT_X", this);

    apvts.addParameterListener("INITIAL_POINT_Y", this);

    //I also use the callback to update the wavetype instead of doing it inside process block
    //Since it would probably be a bit computationally heavy, and we don't care too much aboute response time
    //for these params
    apvts.addParameterListener("WAVE_TYPE0", this);
    apvts.addParameterListener("WAVE_TYPE1", this);
    apvts.addParameterListener("WAVE_TYPE2", this);
    apvts.addParameterListener("WAVE_TYPE3", this);


}

FractalSynthesisAudioProcessor::~FractalSynthesisAudioProcessor()
{

}

//==============================================================================
const juce::String FractalSynthesisAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FractalSynthesisAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FractalSynthesisAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FractalSynthesisAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FractalSynthesisAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FractalSynthesisAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FractalSynthesisAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FractalSynthesisAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FractalSynthesisAudioProcessor::getProgramName (int index)
{
    return {};
}

void FractalSynthesisAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FractalSynthesisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..


    //Prepare all the voices inside the synth

    for (int i = 0; i < synth->getNumVoices(); i++)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth->getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }

    }
    synth->setCurrentPlaybackSampleRate(sampleRate);

}

void FractalSynthesisAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    synth->clearSounds();
    synth->clearVoices();
  
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FractalSynthesisAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FractalSynthesisAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    std::complex<double> z = 0; //starting z
    std::complex<double> c = 0; //starting point

    if (updatedFractal)
    {
        //Get the starting point for the fractal
        auto& startingX = *apvts.getRawParameterValue("INITIAL_POINT_X");
        auto& startingY = *apvts.getRawParameterValue("INITIAL_POINT_Y");


        float x = startingX.load();
        float y = startingY.load();

        c = std::complex<double>(x, y);

        generateFractalSuccession(c);

        generateLFORates(fractalPoints);

        generateFreqDetunes(fractalPoints);

        updatedFractal = false;
    }


    for (int i = 0; i < synth->getNumVoices(); ++i)
    {
        
        if (auto voice = dynamic_cast<SynthVoice*>(synth->getVoice(i)))
        {
            // Osc controls
            // ADSR
            // LFO ecc...

            for (size_t j = 0; j < processor_consts::NUM_PARTIALS; j++)
            {
                updateADSR(j, voice);
                //Push current voice and partial buffer to wave visualizer
                waveVisualisers[j]->pushBuffer(*voice->synthBuffers[j]);
            }
            voice->setFreqDetunes(freqDetunes);
            voice->setLFORates(lfoRates);
            
        }
    
    }
  
    synth->renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());


    midiMessages.clear();
}

//==============================================================================
bool FractalSynthesisAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FractalSynthesisAudioProcessor::createEditor()
{
    return new FractalSynthesisAudioProcessorEditor (*this);
}

//==============================================================================
void FractalSynthesisAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FractalSynthesisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void FractalSynthesisAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    //Here we update the values of the synths based on the selected fractal function



    if (parameterID.compare("FRACTAL_FUNCTION")==0)
    {
        int fractalOption = newValue;

        switch (fractalOption)
        {
        case 0:
            currentFractal = mandelbrot;
            break;
        case 1:
            currentFractal = burningShip;
            break;
        case 2:
            currentFractal = tricorn;
        default:
            break;
        }

        updatedFractal = true;
    }
    else if (parameterID.compare("INITIAL_POINT_X")==0 || parameterID.compare("INITIAL_POINT_Y")==0)
    {
        updatedFractal = true;
    }


    if (parameterID.contains("WAVE_TYPE"))
    {
        int index = parameterID.getLastCharacter() - '0';
        int newChoice = newValue;

        for (int i = 0; i < synth->getNumVoices(); ++i)
        {
            if (auto voice = dynamic_cast<SynthVoice*>(synth->getVoice(i)))
            {

                voice->setWaveType(index, newChoice);
            }
        }
    }

}

juce::AudioProcessorValueTreeState::ParameterLayout FractalSynthesisAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>("FRACTAL_FUNCTION", "Fractal Function",
        juce::StringArray("Mandelbrot Set", "Burning Ship Set", "Tricorn"), 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("INITIAL_POINT_X", "Initial Point X", -1, 1, 0.5));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("INITIAL_POINT_Y", "Initial Point Y", -1, 1, 0.5));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK0", "Attack",
        juce::NormalisableRange<float> {0.01f, 1.0f, 0.001f}, 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY0", "Decay",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 0.1f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN0", "Sustain",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE0", "Release",
        juce::NormalisableRange<float> {0.1f, 3.0f, 0.001f}, 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("WAVE_TYPE0", "Wave type",
        juce::StringArray("Sine", "Saw", "Square"), 0));



    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK1", "Attack",
        juce::NormalisableRange<float> {0.01f, 1.0f, 0.001f}, 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY1", "Decay",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN1", "Sustain",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE1", "Release",
        juce::NormalisableRange<float> {0.1f, 3.0f, 0.001f}, 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("WAVE_TYPE1", "Wave type",
        juce::StringArray("Sine", "Saw", "Square"), 0));






    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK2", "Attack",
        juce::NormalisableRange<float> {0.01f, 1.0f, 0.001f}, 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY2", "Decay",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN2", "Sustain",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE2", "Release",
        juce::NormalisableRange<float> {0.1f, 3.0f, 0.001f}, 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("WAVE_TYPE2", "Wave type",
        juce::StringArray("Sine", "Saw", "Square"), 0));






    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK3", "Attack",
        juce::NormalisableRange<float> {0.01f, 1.0f, 0.001f}, 0.01f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY3", "Decay",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SUSTAIN3", "Sustain",
        juce::NormalisableRange<float> {0.1f, 1.0f, 0.001f}, 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("RELEASE3", "Release",
        juce::NormalisableRange<float> {0.1f, 3.0f, 0.001f}, 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("WAVE_TYPE3", "Wave type",
        juce::StringArray("Sine", "Saw", "Square"), 0));



    return { params.begin(), params.end() };
}

std::complex<double> FractalSynthesisAudioProcessor::mandelbrot(std::complex<double> z, std::complex<double> c)
{
    return std::pow(z, 2) + c;
}

std::complex<double> FractalSynthesisAudioProcessor::burningShip(std::complex<double> z, std::complex<double> c)
{
    std::complex<double> z1(std::abs(z.real()), std::abs(z.imag()));
    return std::pow(z1, 2) + c;
}

std::complex<double> FractalSynthesisAudioProcessor::tricorn(std::complex<double> z, std::complex<double> c)
{
    return pow(conj(z), 2) + c;
}


void FractalSynthesisAudioProcessor::generateFractalSuccession(std::complex<double> c)
{

    std::complex<double> z = 0; //starting z

    for (size_t synthNumber = 0; synthNumber < processor_consts::NUM_PARTIALS; synthNumber++)
    {
        z = currentFractal(z, c);
        fractalPoints[synthNumber] = z;
    }
}

void FractalSynthesisAudioProcessor::generateLFORates(std::vector<std::complex<double>> fractalSuccession)
{

    double total = 0;

    for (auto& fractalPoint : fractalSuccession)
            total += std::abs(fractalPoint.imag());

    for (size_t i = 0; i < fractalSuccession.size(); i++)
    {
        lfoRates[i] = std::abs(fractalSuccession[i].imag()) * 10 / total;
    }


}

void FractalSynthesisAudioProcessor::generateFreqDetunes(std::vector<std::complex<double>> fractalSuccession)
{

    freqDetunes[0] = 1; //Always keep the fundamental unchanged
    for (size_t i = 1; i < fractalSuccession.size(); i++)
    {
        freqDetunes[i] = std::abs(fractalSuccession[i].real());
    }
}

void FractalSynthesisAudioProcessor::updateADSR(int partialIndex, SynthVoice* voice)
{
    auto indexString = std::to_string(partialIndex);

    auto& attack = *apvts.getRawParameterValue("ATTACK" + indexString);
    auto& decay = *apvts.getRawParameterValue("DECAY" + indexString);
    auto& sustain = *apvts.getRawParameterValue("SUSTAIN" + indexString);
    auto& release = *apvts.getRawParameterValue("RELEASE" + indexString);

    voice->updateADSR(partialIndex, attack.load(), decay.load(), sustain.load(), release.load());
}





//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractalSynthesisAudioProcessor();
}
