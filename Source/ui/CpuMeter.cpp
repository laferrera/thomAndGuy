#include "CpuMeter.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

CpuMeter::CpuMeter (ThomAndGuyAudioProcessor& p) : processor (p)
{
    startTimerHz (10);
}

CpuMeter::~CpuMeter() = default;

void CpuMeter::timerCallback()
{
    const float raw = processor.getCpuLoad() * 100.0f;
    displayCpu = displayCpu * 0.9f + raw * 0.1f;
    repaint();
}

void CpuMeter::paint (juce::Graphics& g)
{
    if (auto* tgLnf = dynamic_cast<ThomAndGuyLookAndFeel*> (&getLookAndFeel()))
        g.setFont (tgLnf->getMonoFont (10.0f));
    else
        g.setFont (10.0f);

    g.setColour (displayCpu > 80.0f ? BNT::magenta : BNT::nodeGrey);
    const auto text = "CPU " + juce::String (static_cast<int> (displayCpu)) + "%";
    g.drawText (text, getLocalBounds(), juce::Justification::centredRight, false);
}
