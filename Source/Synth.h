/*
  ==============================================================================

    Synth.h
    Created: 25 Jul 2025 11:09:59pm
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "NoiseGenerator.h"
#include "Parameters.h"

class Synth {
    public:
        Synth();
        // These two functions are called right before the host starts playoing anduio and after it finishes
        // Think of them like prepareToPlay and releaseResources
        void allocateResources(double sampleRate, int samplesPerBlock);
        void deallocateResources();
        void reset();
        void render(float** outputBuffers,  int sampleCount);
        void midiMessage(uint8_t data0, uint8_t data1, uint8_t data2);
        Parameters params;

    private:
        float sampleRate;
        Voice voice;
        NoiseGenerator noiseGen;
        void noteOn(int note, int velocity);
        void noteOff(int note);
        float calcPeriod(int midiNote) const;
};
