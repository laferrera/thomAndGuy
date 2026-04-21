#include "MainPanel.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

static float getParamDefault (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& id)
{
    auto* p = apvts.getParameter (id);
    return p != nullptr ? p->convertFrom0to1 (p->getDefaultValue()) : 0.0f;
}

MainPanel::MainPanel (ThomAndGuyAudioProcessor& p)
    : processor (p),
      modeSwitch (p.apvts, ParamIDs::filterMode, "Envelope", "Formant"),
      envelopeMeter (p),
      presetBar (p),
      cpuMeter (p)
{
    auto& apvts = p.apvts;

    auto addKnob = [&apvts] (KnobGroup& group,
                             ValueSlider& slider,
                             std::unique_ptr<Attachment>& att,
                             const juce::String& paramID,
                             const juce::String& shortLabel,
                             const juce::String& tooltipLabel)
    {
        group.addSlider (slider, shortLabel);
        att = std::make_unique<Attachment> (apvts, paramID, slider);
        slider.setDoubleClickReturnValue (true, getParamDefault (apvts, paramID));
        slider.setParamLabel (tooltipLabel);
    };

    addKnob (inputGroup,    inputGainSlider,    inputGainAtt,    ParamIDs::inputGain,    "Gain",    "Input Gain");

    addKnob (envelopeGroup, sensitivitySlider,  sensitivityAtt,  ParamIDs::sensitivity,  "Sens",    "Sensitivity");
    addKnob (envelopeGroup, attackSlider,       attackAtt,       ParamIDs::attack,       "Atk",     "Attack");
    addKnob (envelopeGroup, rangeSlider,        rangeAtt,        ParamIDs::range,        "Range",   "Range");
    addKnob (envelopeGroup, decaySlider,        decayAtt,        ParamIDs::decay,        "Decay",   "Decay");

    addKnob (driveGroup,    driveSlider,        driveAtt,        ParamIDs::drive,        "Drive",   "Drive");
    addKnob (driveGroup,    morphSlider,        morphAtt,        ParamIDs::morph,        "Morph",   "Morph");
    addKnob (driveGroup,    subBlendSlider,     subBlendAtt,     ParamIDs::subBlend,     "Sub",     "Sub Blend");

    addKnob (outputGroup,   wetDrySlider,       wetDryAtt,       ParamIDs::wetDry,       "Wet/Dry", "Wet/Dry");
    addKnob (outputGroup,   outputLevelSlider,  outputLevelAtt,  ParamIDs::outputLevel,  "Level",   "Output Level");

    addAndMakeVisible (inputGroup);
    addAndMakeVisible (envelopeGroup);
    addAndMakeVisible (driveGroup);
    addAndMakeVisible (outputGroup);

    // Envelope-mode cluster
    addKnob (envelopeModeGroup, baseCutoffSlider, baseCutoffAtt, ParamIDs::baseCutoff, "Cutoff",  "Base Cutoff");
    addKnob (envelopeModeGroup, envAmountSlider,  envAmountAtt,  ParamIDs::envAmount,  "Env Amt", "Env Amount");
    filterTypeBox.addItemList (ParamIDs::filterTypeChoices, 1);
    filterTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::filterType, filterTypeBox);
    envelopeModeGroup.addAndMakeVisible (filterTypeBox);

    // Formant-mode cluster
    vowelABox.addItemList (ParamIDs::vowelChoices, 1);
    vowelBBox.addItemList (ParamIDs::vowelChoices, 1);
    stretchCurveBox.addItemList (ParamIDs::stretchCurveChoices, 1);
    vowelAAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::vowelA, vowelABox);
    vowelBAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::vowelB, vowelBBox);
    stretchCurveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, ParamIDs::stretchCurve, stretchCurveBox);
    addKnob (formantModeGroup, formantDepthSlider, formantDepthAtt, ParamIDs::formantDepth, "Depth", "Formant Depth");
    formantModeGroup.addAndMakeVisible (vowelABox);
    formantModeGroup.addAndMakeVisible (vowelBBox);
    formantModeGroup.addAndMakeVisible (stretchCurveBox);

    addAndMakeVisible (modeSwitch);
    addAndMakeVisible (envelopeModeGroup);
    addAndMakeVisible (formantModeGroup);
    addAndMakeVisible (envelopeMeter);
    addAndMakeVisible (presetBar);
    addAndMakeVisible (cpuMeter);

    modeSwitch.onChange = [this] (int ix) { applyFilterMode (ix); };

    const int initialMode = (int) *apvts.getRawParameterValue (ParamIDs::filterMode);
    applyFilterMode (initialMode);
}

MainPanel::~MainPanel() = default;

void MainPanel::applyFilterMode (int modeIndex)
{
    const bool envelopeActive = (modeIndex == 0);
    envelopeModeGroup.setVisible (envelopeActive);
    formantModeGroup .setVisible (! envelopeActive);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (BNT::black0);

    // Header band
    auto area = getLocalBounds();
    auto header = area.removeFromTop (52);
    g.setColour (BNT::black1);
    g.fillRect (header);
    g.setColour (BNT::cream);

    if (auto* tgLnf = dynamic_cast<ThomAndGuyLookAndFeel*> (&getLookAndFeel()))
        g.setFont (tgLnf->getMonoFont (20.0f));
    else
        g.setFont (20.0f);

    g.drawFittedText ("&& THOM & GUY", header.reduced (16, 0),
                       juce::Justification::centredLeft, 1);
}

void MainPanel::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (52).reduced (16, 12);
    envelopeMeter.setBounds (header.removeFromRight (header.getWidth() / 3));
    header.removeFromRight (12);
    presetBar.setBounds (header.removeFromRight (150));
    header.removeFromRight (12);
    cpuMeter.setBounds (header.removeFromRight (60));
    area.reduce (16, 12);

    // Top row: Input | Envelope Feel | Drive
    auto topRow = area.removeFromTop (96);
    const int topTotal = topRow.getWidth();
    inputGroup   .setBounds (topRow.removeFromLeft (topTotal / 5));
    topRow.removeFromLeft (8);
    envelopeGroup.setBounds (topRow.removeFromLeft ((topTotal * 2) / 5));
    topRow.removeFromLeft (8);
    driveGroup   .setBounds (topRow);

    area.removeFromTop (16); // divider gap

    // Middle area: mode switch + mode-specific knobs
    auto modeRow = area.removeFromTop (36);
    modeSwitch.setBounds (modeRow);

    area.removeFromTop (8);

    auto clusterRow = area.removeFromTop (80);
    envelopeModeGroup.setBounds (clusterRow);
    formantModeGroup .setBounds (clusterRow);

    area.removeFromTop (16);

    // Bottom row: Output (full width)
    auto bottomRow = area.removeFromTop (96);
    outputGroup.setBounds (bottomRow);

    // Manually place ComboBoxes inside their respective groups (local coords).
    {
        auto r = envelopeModeGroup.getLocalBounds().reduced (6, 24);
        filterTypeBox.setBounds (r.removeFromRight (60).withHeight (24).withCentre (r.getCentre()));
    }
    {
        auto r = formantModeGroup.getLocalBounds().reduced (6, 24);
        const int third = r.getWidth() / 3;
        vowelABox      .setBounds (r.removeFromLeft (third - 4).withHeight (24));
        r.removeFromLeft (4);
        vowelBBox      .setBounds (r.removeFromLeft (third - 4).withHeight (24));
        r.removeFromLeft (4);
        stretchCurveBox.setBounds (r.removeFromLeft (third - 4).withHeight (24));
    }
}
