#pragma once

#include <JuceHeader.h>

class ThomAndGuyAudioProcessor;

class EnvelopeMeter : public juce::Component, private juce::Timer
{
public:
    explicit EnvelopeMeter (ThomAndGuyAudioProcessor& p);
    ~EnvelopeMeter() override;

    void paint   (juce::Graphics&) override;

private:
    void timerCallback() override;

    ThomAndGuyAudioProcessor& processor;
    float smoothedEnv = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeMeter)
};
