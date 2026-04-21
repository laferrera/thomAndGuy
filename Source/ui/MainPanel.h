#pragma once

#include <JuceHeader.h>
#include "KnobGroup.h"

class ThomAndGuyAudioProcessor;

class MainPanel : public juce::Component
{
public:
    explicit MainPanel (ThomAndGuyAudioProcessor& p);
    ~MainPanel() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;

private:
    ThomAndGuyAudioProcessor& processor;

    KnobGroup inputGroup      { "Input" };
    KnobGroup envelopeGroup   { "Envelope Feel" };
    KnobGroup driveGroup      { "Drive" };
    KnobGroup outputGroup     { "Output" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
