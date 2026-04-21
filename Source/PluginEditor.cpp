#include "PluginEditor.h"

ThomAndGuyAudioProcessorEditor::ThomAndGuyAudioProcessorEditor (ThomAndGuyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mainPanel (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (mainPanel);
    setSize (600, 360);
}

ThomAndGuyAudioProcessorEditor::~ThomAndGuyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ThomAndGuyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (BNT::black0);
}

void ThomAndGuyAudioProcessorEditor::resized()
{
    mainPanel.setBounds (getLocalBounds());
}
