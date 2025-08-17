/*
  ==============================================================================

    Parameters.h
    Created: 26 Jul 2025 6:00:32pm
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Preset.h"

template<typename T>
inline static void castParameter(juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& id, T& destination) {
    destination = dynamic_cast<T>(apvts.getParameter(id.getParamID()));
    // Parameter does not exist or is wrong type
    jassert(destination);
}

namespace ParameterID {

    #define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

    PARAMETER_ID(oscMix)
    PARAMETER_ID(oscTune)
    PARAMETER_ID(oscFine)
    PARAMETER_ID(glideMode)
    PARAMETER_ID(glideRate)
    PARAMETER_ID(glideBend)
    PARAMETER_ID(filterFreq)
    PARAMETER_ID(filterReso)
    PARAMETER_ID(filterEnv)
    PARAMETER_ID(filterLFO)
    PARAMETER_ID(filterVelocity)
    PARAMETER_ID(filterAttack)
    PARAMETER_ID(filterDecay)
    PARAMETER_ID(filterSustain)
    PARAMETER_ID(filterRelease)
    PARAMETER_ID(envAttack)
    PARAMETER_ID(envDecay)
    PARAMETER_ID(envSustain)
    PARAMETER_ID(envRelease)
    PARAMETER_ID(lfoRate)
    PARAMETER_ID(vibrato)
    PARAMETER_ID(noise)
    PARAMETER_ID(octave)
    PARAMETER_ID(tuning)
    PARAMETER_ID(outputLevel)
    PARAMETER_ID(polyMode)

    #undef PARAMETER_ID
}

static constexpr int MAX_VOICES = 8;

class Parameters {
    public:
        void initParams(juce::AudioProcessorValueTreeState& apvts);
        void updateParams(float sampleRate);
        void setCurrentProgram(int index);
        void createDefaultPresets();
        void reset(float sampleRate);
        float oscMix;
        float detune;
        float tune;
        float velocitySensitivity;
        bool ignoreVelocity;
        float lfoInc;
        float vibratoAmount;
        float pwmDepth;
        float envAttack, envDecay, envSustain, envRelease;
        float volumeTrim;   // Used to automatically adjust the volume
        juce::LinearSmoothedValue<float> outputLevelSmoother;
        float noiseMix;
        float pitchBend;
        float modWheel;
        int glideMode;
        float glideRate;
        float glideBend;
        int numVoices;
        static constexpr float ANALOG = 0.002f;
        std::vector<Preset> presets;
        int totalPresets() {
            return int(presets.size());
        }
    private:
        // All the parameter objects (pointers)
        juce::AudioParameterFloat* oscMixParam;
        juce::AudioParameterFloat* oscTuneParam;
        juce::AudioParameterFloat* oscFineParam;
        juce::AudioParameterChoice* glideModeParam;
        juce::AudioParameterFloat* glideRateParam;
        juce::AudioParameterFloat* glideBendParam;
        juce::AudioParameterFloat* filterFreqParam;
        juce::AudioParameterFloat* filterResoParam;
        juce::AudioParameterFloat* filterEnvParam;
        juce::AudioParameterFloat* filterLFOParam;
        juce::AudioParameterFloat* filterVelocityParam;
        juce::AudioParameterFloat* filterAttackParam;
        juce::AudioParameterFloat* filterDecayParam;
        juce::AudioParameterFloat* filterSustainParam;
        juce::AudioParameterFloat* filterReleaseParam;
        juce::AudioParameterFloat* envAttackParam;
        juce::AudioParameterFloat* envDecayParam;
        juce::AudioParameterFloat* envSustainParam;
        juce::AudioParameterFloat* envReleaseParam;
        juce::AudioParameterFloat* lfoRateParam;
        juce::AudioParameterFloat* vibratoParam;
        juce::AudioParameterFloat* noiseParam;
        juce::AudioParameterFloat* octaveParam;
        juce::AudioParameterFloat* tuningParam;
        juce::AudioParameterFloat* outputLevelParam;
        juce::AudioParameterChoice* polyModeParam;
};
