#include "PluginEditor.h"

ThomAndGuyAudioProcessorEditor::ThomAndGuyAudioProcessorEditor (ThomAndGuyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 360);
}

void ThomAndGuyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff000000));
    g.setColour (juce::Colour (0xfffffffe));
    g.setFont (20.0f);
    g.drawText ("THOM & GUY", getLocalBounds(), juce::Justification::centred, true);
}

void ThomAndGuyAudioProcessorEditor::resized() {}
