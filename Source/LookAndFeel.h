/*
  ==============================================================================

    LookAndFeel.h
    Created: 25 Aug 2025 8:42:31am
    Author:  Paul Mayer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
class LookAndFeel : public juce::LookAndFeel_V4 {
    public:
        LookAndFeel();
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LookAndFeel)
};
