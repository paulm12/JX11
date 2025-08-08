/*
  ==============================================================================

    Envelope.h
    Created: 2 Aug 2025 1:17:21am
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once
// Corresponds to -80dB; -20Log10(0.0001f)
const float SILENCE = 0.0001f;// What the multiplier formula looks like
//decayTime = 2.0
//decaySamples = sampleRate * decayTime m
//multiplier = exp(log(0.0001) / decaySamples)

class Envelope {
    private:
        float target;
        float multiplier;
    
    public:
        float level;
        float attackMultiplier;
        float decayMultiplier;
        float sustainLevel;
        float releaseMultiplier;
    
        void reset() {
            level = 0.0f;
            target = 0.0f;
            multiplier = 0.0f;
        }
        
        float nextValue() {
//            level *= multiplier;
//            level *= 0.9999f;
            // With smoothing:
            level = multiplier * (level - target) + target;
            // Trigger the decay stage if attack finishes
            // The "target" is 2 (for more punch in the attack stage)
            if (level + target > 3.0f) {
                multiplier = decayMultiplier;
                target = sustainLevel;
            }
            return level;
        }
    
        inline bool isActive() const noexcept {
            return level > SILENCE;
        }
    
        inline bool isInAttackStage() const {
            return target >= 2.0f;
        }
    
        void attack() {
            level += SILENCE + SILENCE;
            target = 2.0f;
            multiplier = attackMultiplier;
        }
    
        void release() {
            target = 0.0f;
            multiplier = releaseMultiplier;
        }
};
