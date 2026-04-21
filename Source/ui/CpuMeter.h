#pragma once

#include <JuceHeader.h>

class ThomAndGuyAudioProcessor;

class CpuMeter : public juce::Component, private juce::Timer
{
public:
    explicit CpuMeter (ThomAndGuyAudioProcessor& p);
    ~CpuMeter() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    ThomAndGuyAudioProcessor& processor;
    float displayCpu = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CpuMeter)
};
