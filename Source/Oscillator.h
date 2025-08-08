/*
  ==============================================================================

    Oscillator.h
    Created: 26 Jul 2025 2:18:52am
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once
#include <cmath>

const float PI_OVER_4 = 0.7853981633974483f;
const float PI = 3.1415926535897932f;
const float TWO_PI = 6.2831853071795864f;

class Oscillator {
    // Here phase counts from 0 up to (period /2)* pi to render the first have of the sinc, and counts back down again to 0 to render the second half
    public:
        float period = 0.0f;
        float amplitude = 1.0f;
    
    void reset() {
        inc = 0.0f;
        phase = 0.0f;
        sin0 = 0.0f;
        sin1 = 0.0f;
        dsin = 0.0f;
    }
    
    float nextSample() {
        float output = 0.0f;
        // Update the phase
        phase += inc;
        // Here, the oscilaltor should start a new impulse)
        if (phase <= PI_OVER_4) {
            // Find the midpoint between the peak that just finished and the new one (depends on the period).  Ignores any change to period until the next cycle stops.
            float halfPeriod = period / 2.0f;
            phaseMax = std::floor(0.5f + halfPeriod) - 0.5f;
            dc = 0.5f * amplitude / phaseMax;
            phaseMax *= PI;
            
            inc = phaseMax / halfPeriod;
            phase = -phase;
            // Calculate the sinc (where phase holds the current pi * x)
            // Avoid dividing by zero (phase could be negative, so we can just do it this way
            sin0 = amplitude * std::sin(phase);
            sin1 = amplitude * std::sin(phase - inc);
            dsin = 2.0f * std::cos(inc);
            
            if (phase*phase > 1e-9) {
                output = sin0 / phase;
            } else {
                output = amplitude;
            }
        } else {
            // Current sample is somewhere between previous peak and next
            // If the phase counter goes past the halfway point, set it to maximum and invert it so we output the sinc fuinction backwards
            if (phase > phaseMax) {
                phase = phaseMax + phaseMax - phase;
                inc = -inc;
            }
            // Calculate the sinc function backwards (phase will always be large neough from here)
            float sinp = dsin * sin0 - sin1;
            sin1 = sin0;
            sin0 = sinp;
            output = sinp / phase;
        }
        return output - dc;
    }
    
    private:
        float phase;
        // phaseMax is the midpoint between the two impulse peaks (measured in samples times pi
        float phaseMax;
        float inc;
        float sin0, sin1, dsin;
        float dc;
};
