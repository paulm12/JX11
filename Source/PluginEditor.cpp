/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JX11AudioProcessorEditor::JX11AudioProcessorEditor (JX11AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&globalLNF);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be
    outputLevelKnob.label = "Level";
    addAndMakeVisible(outputLevelKnob);
    filterResoKnob.label = "Reso";
    addAndMakeVisible(filterResoKnob);
    
    polyModeButton.setButtonText("Poly");
    polyModeButton.setClickingTogglesState(true);
    addAndMakeVisible(polyModeButton);
    // MIDI Learn Button:
    midiLearnButton.setButtonText("MIDI Learn");
    midiLearnButton.addListener(this);
    addAndMakeVisible(midiLearnButton);
    // Should be done at the end
    setSize (600, 400);
}

JX11AudioProcessorEditor::~JX11AudioProcessorEditor()
{
    midiLearnButton.removeListener(this);
    audioProcessor.midiLearn = false;
}

//==============================================================================
void JX11AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//    g.setColour (juce::Colours::white);
//    g.setFont (juce::FontOptions (15.0f));
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void JX11AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor.
    int knobwidth = 100;
    int padding = 20;
    juce::Rectangle r(padding, padding, knobwidth, knobwidth + 20);
    outputLevelKnob.setBounds(r);
    
    r = r.withX(r.getRight() + padding);
    filterResoKnob.setBounds(r);
    
    int textButtonWidth = 80;
    int textButtonHeight = 30;
    polyModeButton.setSize(textButtonWidth, textButtonHeight);
    polyModeButton.setCentrePosition(r.withX(r.getRight()).getCentre());
    // For the MIDI learn button:
    midiLearnButton.setBounds(400, 20, 100, 30);
}

void JX11AudioProcessorEditor::buttonClicked(juce::Button* button) {
    button->setButtonText("Waiting...");
    button->setEnabled(false);
    audioProcessor.midiLearn = true;
    startTimerHz(10);
}

void JX11AudioProcessorEditor::timerCallback() {
    if (!audioProcessor.midiLearn) {
        stopTimer();
        midiLearnButton.setButtonText("MIDI Learn");
        midiLearnButton.setEnabled(true);
    }
}
