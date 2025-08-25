/*
  ==============================================================================

    RotaryKnob.h
    Created: 24 Aug 2025 7:38:57pm
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class RotaryKnob : public juce::Component {
    public:
        RotaryKnob();
        ~RotaryKnob() override;
        void paint(juce::Graphics&) override;
        void resized() override;
        juce::Slider slider;
        juce::String label;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};
