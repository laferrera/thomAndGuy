#pragma once

#include <JuceHeader.h>

class ThomAndGuyAudioProcessor;

class PresetBar : public juce::Component
{
public:
    explicit PresetBar (ThomAndGuyAudioProcessor& p);
    ~PresetBar() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;

private:
    ThomAndGuyAudioProcessor& processor;
    juce::ComboBox dropdown;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
