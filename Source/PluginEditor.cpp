#include "PluginEditor.h"

ThomAndGuyAudioProcessorEditor::ThomAndGuyAudioProcessorEditor (ThomAndGuyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 360);
    setLookAndFeel (&lnf);
}

ThomAndGuyAudioProcessorEditor::~ThomAndGuyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ThomAndGuyAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace BNT;
    g.fillAll (black0);
    g.setColour (cream);
    g.setFont (20.0f);
    g.drawText ("THOM & GUY", getLocalBounds(), juce::Justification::centred, true);
}

void ThomAndGuyAudioProcessorEditor::resized() {}
