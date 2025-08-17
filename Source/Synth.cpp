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
    for (int v = 0; v < MAX_VOICES; v++) {
        voices[v].reset();
    }
    noiseGen.reset();
    sustainPedalPressed = false;
    params.reset(sampleRate);
    lfo = 0.0f;
    lfoStep = 0;
}

void Synth::render(float **outputBuffers, int sampleCount) {
    // Renders a naked float pointer
    // We could use a juce::AudioBuffer if we wanted to
    float* outputBufferLeft = outputBuffers[0];
    float* outputBufferRight = outputBuffers[1];
        
    for (int v = 0; v < MAX_VOICES; ++v) {
        Voice& voice = voices[v];
        if (voice.env.isActive()) {
            voice.osc1.period = voice.period * params.pitchBend;
            voice.osc2.period = voice.osc1.period * params.detune;
        }
    }
    
    for (int sample = 0; sample < sampleCount; ++sample) {
        updateLFO();
        const float noise = noiseGen.nextValue() * params.noiseMix;

        float outputLeft = 0.0f;
        float outputRight = 0.0f;
        for (int v = 0; v < MAX_VOICES; ++v) {
            Voice& voice = voices[v];
            if (voice.env.isActive()) {
                float output = voice.render(noise);
                outputLeft += output * voice.panLeft;
                outputRight += output * voice.panRight;
            }
        }
        // Adjust the gain
        float outputLevel = params.outputLevelSmoother.getNextValue();
        outputLeft *= outputLevel;
        outputRight *= outputLevel;
            
        if (outputBufferRight != nullptr) {
            outputBufferLeft[sample] = outputLeft;
            outputBufferRight[sample] = outputRight;
        } else {
            outputBufferLeft[sample] = (outputLeft + outputRight) * 0.5f;
        }
    }
    // Turn off the synth (don't render) if the envelope dips down
    for (int v = 0; v < MAX_VOICES; ++v) {
        Voice& voice = voices[v];
        if (!voice.env.isActive()) {
            voice.env.reset();
        }
    }

    protectYourEars(outputBufferLeft, sampleCount);
    protectYourEars(outputBufferRight, sampleCount);
}

void Synth::updateLFO() {
    if (--lfoStep <= 0) {
        lfoStep = LFO_MAX;
    }
    lfo += params.lfoInc;
    if (lfo > PI) {
        lfo -= TWO_PI;
    }
    const float sine = std::sin(lfo);
    float vibratoMod = 1.0f + sine * (params.modWheel + params.vibratoAmount);
    float pwm = 1.0f + sine * (params.modWheel + params.pwmDepth);
    for (int v = 0; v < MAX_VOICES; ++v) {
        Voice& voice = voices[v];
        if (voice.env.isActive()) {
            voice.osc1.pitchModulation = vibratoMod;
            voice.osc2.pitchModulation = pwm;
        }
    }
}

void Synth::midiMessage(uint8_t data0, uint8_t data1, uint8_t data2) {
    switch (data0 & 0xF0) {
        // Pitch Bend
        case 0xE0:
            params.pitchBend = std::exp(-0.000014102f * float(data1 + 128 * data2 - 8192));
            break;
        // Control Change
        case 0xB0:
            controlChange(data1, data2);
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

void Synth::startNote(int v, int note, int velocity) {
    float period = calcPeriod(v, note);
    Voice& voice = voices[v];
    voice.period = period;
    voice.note = note;
    voice.updatePanning();
    // Old way:
//    voice.osc1.amplitude = (params.volumeTrim * velocity / 127.0f) * 0.5f;
    float vel = 0.004f * ((velocity + 64) * (velocity + 64)) - 8.0f;
    voice.osc1.amplitude = params.volumeTrim * vel;
    voice.osc2.amplitude = voice.osc1.amplitude * params.oscMix;
    // If in PWM mode, phase lock oscillator 2
    if (params.vibratoAmount == 0.0f && params.pwmDepth > 0.0f) {
        voice.osc2.squareWave(voice.osc1, voice.period);
    }
    
    voice.env.attackMultiplier = params.envAttack;
    voice.env.decayMultiplier = params.envDecay;
    voice.env.sustainLevel = params.envSustain;
    voice.env.releaseMultiplier = params.envRelease;
    voice.env.attack();
}

void Synth::noteOn(int note, int velocity) {
    // Index of the voice to use
    int v = 0;
    
    if (params.ignoreVelocity) {
        velocity = 80;
    }
    
    if (params.numVoices == 1) {
        // Polyphonic
        if (voices[0].note > 0) {
            shiftQueuedNotes();
            restartMonoVoice(note, velocity);
            return;
        }
    } else {
        v = findFreeVoice();
    }
    startNote(v, note, velocity);
}

void Synth::noteOff(int note) {
    if (params.numVoices == 1 && voices[0].note == note) {
        int queuedNote = nextQueuedNote();
        if (queuedNote > 0) {
            restartMonoVoice(queuedNote, -1);
        }
    }
    for (int v = 0; v < MAX_VOICES; v++) {
        if (voices[v].note == note) {
            if (sustainPedalPressed) {
                voices[v].note = SUSTAIN;
            } else {
                voices[v].release();
                voices[v].note = 0;
            }
        }
    }
}

void Synth::restartMonoVoice(int note, int velocity) {
    float period = calcPeriod(0, note);
    Voice& voice = voices[0];
    voice.period = period;
    
    voice.env.level = SILENCE + SILENCE;
    voice.note = note;
    voice.updatePanning();
}

void Synth::controlChange(uint8_t data1, uint8_t data2) {
    switch (data1) {
        // Mod Wheel
        case 0x01:
            params.modWheel = 0.000005f * float(data2 * data2);
            break;
        // Sustain Pedal
        case 0x40:
            sustainPedalPressed = (data2 >= 64);
            if (!sustainPedalPressed) {
                noteOff(SUSTAIN);
            }
            break;
        default:
            if (data1 >= 0x78) {
                for (int v = 0; v < MAX_VOICES; ++v) {
                    voices[v].reset();
                }
                sustainPedalPressed = false;
            }
            break;
    }
}

float Synth::calcPeriod(int v, int midiNote) const {
    float period = params.tune * std::exp(-0.05776226505f * (float(midiNote) + params.ANALOG * float(v)));
    // Keep the period from being too small, or BILT may not work reliably
    // Makes sure period is at least 6 samples,
    while (period < 6.0f || (period * params.detune) < 6.0f) {
        period += period;
    }
    return period;
}

int Synth::findFreeVoice() const {
    int v = 0;
    float l = 100.0f; // Louder than any envelope!
    
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i].env.level < l && !voices[i].env.isInAttackStage()) {
            l = voices[i].env.level;
            v = i;
        }
    }
    return v;
}

void Synth::shiftQueuedNotes() {
    for (int tmp = MAX_VOICES - 1; tmp > 0; tmp--) {
        voices[tmp].note = voices[tmp - 1].note;
        voices[tmp].release();
    }
}

int Synth::nextQueuedNote() {
    int held = 0;
    for (int v = MAX_VOICES - 1; v > 0; v--) {
        if (voices[v].note > 0) {
            held = v;
        }
    }
    if (held > 0) {
        int note = voices[held].note;
        voices[held].note = 0;
        return note;
    }
    
    return 0;
}
