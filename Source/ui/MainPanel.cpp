#include "MainPanel.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

MainPanel::MainPanel (ThomAndGuyAudioProcessor& p) : processor (p)
{
    addAndMakeVisible (inputGroup);
    addAndMakeVisible (envelopeGroup);
    addAndMakeVisible (driveGroup);
    addAndMakeVisible (outputGroup);
}

MainPanel::~MainPanel() = default;

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
    area.removeFromTop (52); // header
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

    // Middle area reserved for mode switch + mode-specific knobs (Task 18).
    area.removeFromTop (120);

    area.removeFromTop (16);

    // Bottom row: Output (aligned right, half-width)
    auto bottomRow = area.removeFromTop (80);
    outputGroup.setBounds (bottomRow.removeFromRight (bottomRow.getWidth() / 2));
}
