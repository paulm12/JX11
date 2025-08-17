/*
  ==============================================================================

    Voice.h
    Created: 25 Jul 2025 11:09:51pm
    Author:  Paul Mayer

  ==============================================================================
*/
#pragma once

#include "Oscillator.h"
#include "Envelope.h"

struct Voice {
  
    int note;
    float saw;
    float period;
    float targetPeriod;
    float glideRate;
    float panLeft, panRight;
    Envelope env;
    Oscillator osc1, osc2;
    
    void reset() {
        note = 0;
        saw = 0.0f;
        panLeft = 0.707f;
        panRight = 0.707f;
        osc1.reset();
        osc2.reset();
        env.reset();
    }
    
    float render(float input) {
        float sample1 = osc1.nextSample();
        float sample2 = osc2.nextSample();
        // Multiplication by 0.997 is a "leaky" integrator and acts as a simple lowpass filter
        // Plus or minus here is a phase inversion:
        saw = (saw * 0.997f) + (sample1 - sample2);
        float output = saw + input;
        // Apply the envelope
        float envValue = env.nextValue();
        return output * envValue;
    }
    
    void release() {
        env.release();
    }
    
    void updatePanning() {
        float panning = std::clamp((note - 60.0f) / 24.0f, -1.0f, 1.0f);
        panLeft = std::sin(PI_OVER_4 * (1.0f - panning));
        panRight = std::sin(PI_OVER_4 * (1.0f + panning));
    }
    
    void updateLFO() {
        period += glideRate * (targetPeriod - period);
    }
};
