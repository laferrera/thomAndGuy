#pragma once

#include <JuceHeader.h>
#include "KnobGroup.h"
#include "../params/ParameterIDs.h"

class ThomAndGuyAudioProcessor;

class MainPanel : public juce::Component
{
public:
    explicit MainPanel (ThomAndGuyAudioProcessor& p);
    ~MainPanel() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    ThomAndGuyAudioProcessor& processor;

    // Input
    juce::Slider inputGainSlider;
    std::unique_ptr<Attachment> inputGainAtt;

    // Envelope Feel
    juce::Slider sensitivitySlider, attackSlider, rangeSlider, decaySlider;
    std::unique_ptr<Attachment> sensitivityAtt, attackAtt, rangeAtt, decayAtt;

    // Drive
    juce::Slider driveSlider, morphSlider, subBlendSlider;
    std::unique_ptr<Attachment> driveAtt, morphAtt, subBlendAtt;

    // Output
    juce::Slider wetDrySlider, outputLevelSlider;
    std::unique_ptr<Attachment> wetDryAtt, outputLevelAtt;

    KnobGroup inputGroup      { "Input" };
    KnobGroup envelopeGroup   { "Envelope Feel" };
    KnobGroup driveGroup      { "Drive" };
    KnobGroup outputGroup     { "Output" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
