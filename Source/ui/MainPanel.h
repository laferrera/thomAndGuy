#pragma once

#include <JuceHeader.h>
#include "KnobGroup.h"
#include "ModeSwitch.h"
#include "EnvelopeMeter.h"
#include "PresetBar.h"
#include "ValueSlider.h"
#include "CpuMeter.h"
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
    ValueSlider inputGainSlider;
    std::unique_ptr<Attachment> inputGainAtt;

    // Envelope Feel
    ValueSlider sensitivitySlider, attackSlider, rangeSlider, decaySlider;
    std::unique_ptr<Attachment> sensitivityAtt, attackAtt, rangeAtt, decayAtt;

    // Drive
    ValueSlider driveSlider, morphSlider, subBlendSlider;
    std::unique_ptr<Attachment> driveAtt, morphAtt, subBlendAtt;

    // Output
    ValueSlider wetDrySlider, outputLevelSlider;
    std::unique_ptr<Attachment> wetDryAtt, outputLevelAtt;

    KnobGroup inputGroup      { "Input" };
    KnobGroup envelopeGroup   { "Envelope Feel" };
    KnobGroup driveGroup      { "Drive" };
    KnobGroup outputGroup     { "Output" };

    ModeSwitch modeSwitch;
    EnvelopeMeter envelopeMeter;
    PresetBar presetBar;
    CpuMeter cpuMeter;

    // Envelope-mode cluster
    ValueSlider baseCutoffSlider, envAmountSlider;
    juce::ComboBox filterTypeBox;
    std::unique_ptr<Attachment> baseCutoffAtt, envAmountAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAtt;
    KnobGroup envelopeModeGroup { "Envelope Mode" };

    // Formant-mode cluster
    juce::ComboBox vowelABox, vowelBBox, stretchCurveBox;
    ValueSlider formantDepthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> vowelAAtt, vowelBAtt, stretchCurveAtt;
    std::unique_ptr<Attachment> formantDepthAtt;
    KnobGroup formantModeGroup { "Formant Mode" };

    void applyFilterMode (int modeIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
