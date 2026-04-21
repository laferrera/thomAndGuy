#pragma once

#include <JuceHeader.h>

class ModeSwitch : public juce::Component
{
public:
    ModeSwitch (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                juce::StringRef labelA, juce::StringRef labelB);
    ~ModeSwitch() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;

    std::function<void(int)> onChange;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramID;

    juce::TextButton buttonA, buttonB;
    juce::ParameterAttachment valueAttachment;

    void setSelectedIndex (int ix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeSwitch)
};
