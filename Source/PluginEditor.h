#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ThomAndGuyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ThomAndGuyAudioProcessorEditor (ThomAndGuyAudioProcessor&);
    ~ThomAndGuyAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ThomAndGuyAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThomAndGuyAudioProcessorEditor)
};
