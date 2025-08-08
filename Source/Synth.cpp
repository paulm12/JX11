/*
  ==============================================================================

    Synth.cpp
    Created: 25 Jul 2025 11:09:59pm
    Author:  Paul Mayer

  ==============================================================================
*/

#include "Synth.h"
#include "Utils.h"

Synth::Synth() {
    sampleRate = 44100.0f;
}

void Synth::allocateResources(double sampleRate_, int samplesPerBlock) {
    sampleRate = static_cast<float>(sampleRate_);
}

void Synth::deallocateResources() {
    // Nothing yet
}

void Synth::reset() {
    voice.reset();
    noiseGen.reset();
    // Assume the pitch wheel is in the center position when starting
    params.pitchBend = 1.0f;
}

void Synth::render(float **outputBuffers, int sampleCount) {
    // Renders a naked float pointer
    // We could use a juce::AudioBuffer if we wanted to
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];
    
    voice.osc1.period = voice.period * params.pitchBend;
    voice.osc2.period = voice.osc1.period * params.detune;
    
    for (int sample = 0; sample < sampleCount; ++sample) {
        float noise = noiseGen.nextValue() * params.noiseMix;
        
        float outputLeft = 0.0f;
        float outputRight = 0.0f;
        
        float output = 0.0f;
        if (voice.env.isActive()) {
            output = voice.render(noise);
            outputLeft += output * voice.panLeft;
            outputRight += output * voice.panRight;
        }
        
        if (outputBufferRight != nullptr) {
            outputBufferLeft[sample] = outputLeft;
            outputBufferRight[sample] = outputRight;
        } else {
            outputBufferLeft[sample] = (outputLeft + outputRight) * 0.5f;
        }
    }
    // Turn off the synth (don't render) if the envelope dips down
    if (!voice.env.isActive()) {
        voice.env.reset();
    }

    protectYourEars(outputBufferLeft, sampleCount);
    protectYourEars(outputBufferRight, sampleCount);
}

void Synth::midiMessage(uint8_t data0, uint8_t data1, uint8_t data2) {
    switch (data0 & 0xF0) {
        // Pitch Bend
        case 0xE0:
            params.pitchBend = std::exp(-0.000014102f * float(data1 + 128 * data2 - 8192));
            break;
            
        // Note off
        case 0x80:
            noteOff(data1 & 0x7f);
            break;
    
        // Note on
        case 0x90:
            uint8_t note = data1 & 0x7F;
            uint8_t velo = data2 & 0x7F;
            // Ignoring the channel for now
//            uint8_t channel = data0 & 0x0F;
            if (velo > 0) {
                noteOn(note, velo);
            } else {
                noteOff(note);
            }
            break;
            
    }
}

void Synth::noteOn(int note, int velocity) {
    voice.note = note;
    voice.updatePanning();
    
//    Commenting this out, as we made a fn to do this for us
//    float freq = 440.0f * std::exp2((float(note - 69) + params.tune) / 12.0f);
//    voice.period = sampleRate / freq;
    float period = calcPeriod(note);
    voice.period = period;
    
    // Activate the first oscillator
    voice.osc1.amplitude = (velocity / 127.0f) * 0.5f;
    voice.osc2.amplitude = voice.osc1.amplitude * params.oscMix;
    // To reset the oscillators, if desired
//    voice.osc1.reset();
//    voice.osc2.reset();
    
    // Turn the envelope on:
    voice.env.attackMultiplier = params.envAttack;
    voice.env.decayMultiplier = params.envDecay;
    voice.env.sustainLevel = params.envSustain;
    voice.env.releaseMultiplier = params.envRelease;
    voice.env.attack();
    
}

void Synth::noteOff(int note) {
    if (voice.note == note) {
//        voice.note = 0;
        voice.release();
    }
}

float Synth::calcPeriod(int midiNote) const {
    float period = params.tune * std::exp(-0.05776226505f * float(midiNote));
    // Keep the period from being too small, or BILT may not work reliably
    // Makes sure period is at least 6 samples,
    while (period < 6.0f || (period * params.detune) < 6.0f) {
        period += period;
    }
    return period;
}
