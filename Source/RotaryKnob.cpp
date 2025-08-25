/*
  ==============================================================================

    RotaryKnob.cpp
    Created: 24 Aug 2025 7:38:57pm
    Author:  Paul Mayer

  ==============================================================================
*/

#include "RotaryKnob.h"

static constexpr int labelHeight = 15;
static constexpr int textBoxWidth = 100;
static constexpr int textBoxHeight = 20;
static constexpr int knobDim = 100;

RotaryKnob::RotaryKnob() {
    slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);
    // Adjust the start and end radians
    slider.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    
    addAndMakeVisible(slider);
    setBounds(0, 0, textBoxWidth, textBoxHeight + knobDim);
}

RotaryKnob::~RotaryKnob() {
    
}

void RotaryKnob::resized() {
    auto bounds = getLocalBounds();
    // Make room for the label on top, hence why we are subtracting
    slider.setBounds(0, labelHeight, bounds.getWidth(), bounds.getHeight() - labelHeight);
}

void RotaryKnob::paint(juce::Graphics & g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    
    g.setFont(15.0f);
    g.setColour(juce::Colours::white);
    
    auto bounds = getLocalBounds();
    g.drawText(label, juce::Rectangle<int>{0, 0, bounds.getWidth(), labelHeight},  juce::Justification::centred);
    // Give an outline
    g.setColour(juce::Colours::red);
    g.drawRect(getLocalBounds(), 1);
    // For showing the label and slider:
//    g.setColour(juce::Colours::yellow);
//    g.drawRect(0, bounds.getWidth(), bounds.getHeight() - labelHeight - textBoxHeight, 1);
//    g.setColour(juce::Colours::green);
//    g.drawRect(0, 0, bounds.getWidth(), labelHeight, 1);

}
