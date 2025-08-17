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

// For holding down notes while the sustain pedal is pressed
static const int SUSTAIN = -1;

class Synth {
    public:
        Synth();
        Parameters params;
        // These two functions are called right before the host starts playoing anduio and after it finishes
        // Think of them like prepareToPlay and releaseResources
        void allocateResources(double sampleRate, int samplesPerBlock);
        void deallocateResources();
        void reset();
        void render(float** outputBuffers,  int sampleCount);
        void midiMessage(uint8_t data0, uint8_t data1, uint8_t data2);
    private:
        float sampleRate;
        std::array<Voice, MAX_VOICES> voices;
        NoiseGenerator noiseGen;
        int lfoStep;
        float lfo;
        bool sustainPedalPressed;
        void startNote(int v, int note, int velocity);
        void noteOn(int note, int velocity);
        void noteOff(int note);
        void restartMonoVoice(int note, int velocity);
        void controlChange(uint8_t data1, uint8_t data2);
        float calcPeriod(int v, int midiNote) const;
        int findFreeVoice() const;
        void shiftQueuedNotes();
        int nextQueuedNote();
        void updateLFO();
};
