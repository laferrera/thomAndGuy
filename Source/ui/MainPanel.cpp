#include "MainPanel.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

MainPanel::MainPanel (ThomAndGuyAudioProcessor& p)
    : processor (p),
      modeSwitch (p.apvts, ParamIDs::filterMode, "Envelope", "Formant"),
      envelopeMeter (p)
{
    auto& apvts = p.apvts;

    inputGroup.addSlider (inputGainSlider, "Gain");
    inputGainAtt = std::make_unique<Attachment> (apvts, ParamIDs::inputGain, inputGainSlider);

    envelopeGroup.addSlider (sensitivitySlider, "Sens");
    sensitivityAtt = std::make_unique<Attachment> (apvts, ParamIDs::sensitivity, sensitivitySlider);
    envelopeGroup.addSlider (attackSlider, "Atk");
    attackAtt = std::make_unique<Attachment> (apvts, ParamIDs::attack, attackSlider);
    envelopeGroup.addSlider (rangeSlider, "Range");
    rangeAtt = std::make_unique<Attachment> (apvts, ParamIDs::range, rangeSlider);
    envelopeGroup.addSlider (decaySlider, "Decay");
    decayAtt = std::make_unique<Attachment> (apvts, ParamIDs::decay, decaySlider);

    driveGroup.addSlider (driveSlider, "Drive");
    driveAtt = std::make_unique<Attachment> (apvts, ParamIDs::drive, driveSlider);
    driveGroup.addSlider (morphSlider, "Morph");
    morphAtt = std::make_unique<Attachment> (apvts, ParamIDs::morph, morphSlider);
    driveGroup.addSlider (subBlendSlider, "Sub");
    subBlendAtt = std::make_unique<Attachment> (apvts, ParamIDs::subBlend, subBlendSlider);

    outputGroup.addSlider (wetDrySlider, "Wet/Dry");
    wetDryAtt = std::make_unique<Attachment> (apvts, ParamIDs::wetDry, wetDrySlider);
    outputGroup.addSlider (outputLevelSlider, "Level");
    outputLevelAtt = std::make_unique<Attachment> (apvts, ParamIDs::outputLevel, outputLevelSlider);

    addAndMakeVisible (inputGroup);
    addAndMakeVisible (envelopeGroup);
    addAndMakeVisible (driveGroup);
    addAndMakeVisible (outputGroup);

    // Envelope-mode cluster
    envelopeModeGroup.addSlider (baseCutoffSlider, "Cutoff");
    baseCutoffAtt = std::make_unique<Attachment> (apvts, ParamIDs::baseCutoff, baseCutoffSlider);
    envelopeModeGroup.addSlider (envAmountSlider, "Env Amt");
    envAmountAtt = std::make_unique<Attachment> (apvts, ParamIDs::envAmount, envAmountSlider);
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
    formantModeGroup.addSlider (formantDepthSlider, "Depth");
    formantDepthAtt = std::make_unique<Attachment> (apvts, ParamIDs::formantDepth, formantDepthSlider);
    formantModeGroup.addAndMakeVisible (vowelABox);
    formantModeGroup.addAndMakeVisible (vowelBBox);
    formantModeGroup.addAndMakeVisible (stretchCurveBox);

    addAndMakeVisible (modeSwitch);
    addAndMakeVisible (envelopeModeGroup);
    addAndMakeVisible (formantModeGroup);
    addAndMakeVisible (envelopeMeter);

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
    envelopeMeter.setBounds (header.removeFromRight (header.getWidth() / 2));
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

    // Bottom row: Output (aligned right, half-width)
    auto bottomRow = area.removeFromTop (80);
    outputGroup.setBounds (bottomRow.removeFromRight (bottomRow.getWidth() / 2));

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
