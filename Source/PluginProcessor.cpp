/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

//==============================================================================
JX11AudioProcessor::JX11AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // MYR Added: initialize the parameters with the APVTS
    synth.params.initParams(apvts);
    apvts.state.addListener(this);
    // Must be after the APVTS initialization
    synth.params.createDefaultPresets();
    setCurrentProgram(0);
//    juce::String str("Hello World!");
//    DBG(str);
//    std::string test = str.toStdString();
//    DBG(static_cast<int>(test[0]));
}

JX11AudioProcessor::~JX11AudioProcessor()
{
    apvts.state.removeListener(this);
}

//==============================================================================
const juce::String JX11AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JX11AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JX11AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JX11AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JX11AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JX11AudioProcessor::getNumPrograms()
{
    return int(synth.params.presets.size());
}

int JX11AudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void JX11AudioProcessor::setCurrentProgram (int index) {
    currentProgram = index;
    
    synth.params.setCurrentProgram(index);
    
    reset();
}

const juce::String JX11AudioProcessor::getProgramName (int index)
{
    return {synth.params.presets[index].name};
}

void JX11AudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
    // Not implemented
}

//==============================================================================
void JX11AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.allocateResources(sampleRate, samplesPerBlock);
    parametersChanged.store(true);
    reset();
}

void JX11AudioProcessor::releaseResources()
{
    synth.deallocateResources();
}

// MYR added this function
void JX11AudioProcessor::reset() {
    synth.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JX11AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void JX11AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Read the parameters here:
    const juce::String& paramID = ParameterID::noise.getParamID();
    float noiseMix = apvts.getRawParameterValue(paramID) -> load() / 100.0f;
    noiseMix *= noiseMix;
    synth.params.noiseMix = noiseMix * 0.06f;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) { buffer.clear(i, 0, buffer.getNumSamples());
    }
    // For updating the parameters in a thread-safe way:
    bool expected = true;
    if (isNonRealtime() || parametersChanged.compare_exchange_strong(expected, false)) {
        synth.params.updateParams(getSampleRate());
    }
    
    splitBufferByEvents(buffer, midiMessages);
}

// Function added by MYR to split the buffer and handle each MIDI event as it comes in!
void JX11AudioProcessor::splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    int bufferOffset = 0;
    
    for (const auto metadata : midiMessages) {
        // render the audio that happens before the event (if any)
        int samplesThisSegment = metadata.samplePosition - bufferOffset;
        if (samplesThisSegment > 0) {
            render(buffer, samplesThisSegment, bufferOffset);
            bufferOffset += samplesThisSegment;
        }
        
        // Handle the event.  Ignore MIDI messages like sysex:
        if (metadata.numBytes <= 3) {
            uint8_t data1 = (metadata.numBytes >= 2) ? metadata.data[1] : 0;
            uint8_t data2 = (metadata.numBytes == 3) ? metadata.data[2] : 0;
            handleMIDI(metadata.data[0], data1, data2);
        }
    }
    
    // Render thge audio after the last MIDI event.  If there are no MIDI events, render the entire buffer
    int samplesLastSegment = buffer.getNumSamples() - bufferOffset;
    if (samplesLastSegment > 0) {
        render(buffer, samplesLastSegment, bufferOffset);
    }
    midiMessages.clear();
}

// Function added by MYR to deal with the incoming MIDI
void JX11AudioProcessor::handleMIDI(uint8_t data0, uint8_t data1, uint8_t data2) {
//    char s[16];
//    snprintf(s, 16, "%02hhX %02hhX %02hhX", data0, data1, data2);
//    DBG(s);
    
    // Program Change message:
    if ((data0 & 0xF0) == 0xC0) {
        if (data1 < synth.params.totalPresets()) {
            setCurrentProgram(data1);
        }
    }
    
    synth.midiMessage(data0, data1, data2);
}

// Function added by MYR to render audio to the buffer from each MIDI event
void JX11AudioProcessor::render(juce::AudioBuffer<float> &buffer, int sampleCount, int bufferOffset) {
    float* outputBuffers[2] = {nullptr, nullptr};
    outputBuffers[0] = buffer.getWritePointer(0) + bufferOffset;
    // If we need stereo sound
    if (getTotalNumOutputChannels() > 1) {
        outputBuffers[1] = buffer.getWritePointer(1) + bufferOffset;
    }
    
    synth.render(outputBuffers, sampleCount);
}

juce::AudioProcessorValueTreeState::ParameterLayout JX11AudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    //  Add Parameters hewre:
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParameterID::polyMode,
        "Polyphony",
        juce::StringArray {"Mono", "Poly"},
        1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::oscTune,
       "Osc Tune",
       juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f),
       -12.0f,
       juce::AudioParameterFloatAttributes().withLabel("semi")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::oscFine,
       "Osc Fine",
       juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f, 0.3f, true),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("cent")));
    
    auto oscMixStringFromValue = [](float value, int) {
        char s[16] = { 0 };
        snprintf(s, 16, "%4.0f:%2.0f", 100.0 - 0.5f * value, 0.5f * value);
        return juce::String(s);
    };
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::oscMix,
       "Osc Mix",
       juce::NormalisableRange<float>(0.0f, 100.0f),
       0.0f,
       juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(oscMixStringFromValue)));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParameterID::glideMode,
        "Glide Mode",
        juce::StringArray {"Off", "Legato", "Always"},
        0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::glideRate,
       "Glide Rate",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       35.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::glideBend,
       "Glide Bend",
       juce::NormalisableRange<float>(-36.0f, 36.0f, 0.01f, 0.4f, true),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("semi")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterFreq,
       "Filter Cutoff",
       juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
       100.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterReso,
       "Filter Reso",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       15.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterEnv,
       "Filter Env",
       juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
       50.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterLFO,
       "Filter LFO",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    
    auto filterVelocityStringFromValue = [](float value, int) {
        if (value < -90.00f) {
            return juce::String("OFF");
        } else {
            return juce::String(value);
        }
    };
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterVelocity,
       "Filter Velocity",
       juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(filterVelocityStringFromValue)));
    // Filter ADSR Params
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterAttack,
       "Filter Env Attack",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterDecay,
       "Filter Env Decay",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       30.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterSustain,
       "Filter Env Sustain",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::filterRelease,
       "Filter Env Release",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       25.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    // Amp ADSR
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::envAttack,
       "Amp Env Attack",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::envDecay,
       "Amp Env Decay",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       50.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::envSustain,
       "Amp Env Sustain",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       100.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::envRelease,
       "Amp Env Release",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       30.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    // Basic LFO:
    auto lfoRateStringFromValue = [](float value, int) {
        float lfoHz = std::exp(7.0f * value - 4.0f);
        return juce::String(lfoHz, 3);
    };
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::lfoRate,
       "LFO Rate",
       juce::NormalisableRange<float>(),
       0.81f,
       juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction(lfoRateStringFromValue)));
//     Vibrato
    auto vibratoStringFromValue = [](float value, int) {
        if (value < 0.0f) {
            return "PWM " + juce::String(-value, 1);
        } else {
            return juce::String(value, 1);
        }
    };
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::vibrato,
       "Vibrato",
       juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
       0.0f,
       juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction(vibratoStringFromValue)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::noise,
       "Noise Amount",
       juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("%")));
    // Oscillator tuning
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::octave,
       "Octave",
       juce::NormalisableRange<float>(-2.0f, 2.0f, 1.0f),
       0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::tuning,
       "Tuning",
       juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("Cent")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
       ParameterID::outputLevel,
       "Output Level",
       juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
       0.0f,
       juce::AudioParameterFloatAttributes().withLabel("dB")));
    return layout;
}

//==============================================================================
bool JX11AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JX11AudioProcessor::createEditor()
{
//    return new JX11AudioProcessorEditor (*this);
    // MYR Added the following
    auto editor = new juce::GenericAudioProcessorEditor(*this);
    editor->setSize(500, 1050);
    return editor;
}

//==============================================================================
void JX11AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void JX11AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        parametersChanged.store(true);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JX11AudioProcessor();
}
