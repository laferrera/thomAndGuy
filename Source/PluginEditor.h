#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/MainPanel.h"

class ThomAndGuyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ThomAndGuyAudioProcessorEditor (ThomAndGuyAudioProcessor&);
    ~ThomAndGuyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ThomAndGuyAudioProcessor& audioProcessor;
    ThomAndGuyLookAndFeel lnf;
    MainPanel mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThomAndGuyAudioProcessorEditor)
};
