#include "EnvelopeMeter.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

EnvelopeMeter::EnvelopeMeter (ThomAndGuyAudioProcessor& p) : processor (p)
{
    startTimerHz (30);
}

EnvelopeMeter::~EnvelopeMeter() = default;

void EnvelopeMeter::timerCallback()
{
    const float latest = processor.getEnvelopeForUI();
    smoothedEnv += 0.4f * (latest - smoothedEnv);
    repaint();
}

void EnvelopeMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float pad = 2.0f;
    auto track = area.reduced (pad);

    g.setColour (BNT::black3);
    g.fillRect (track);

    const float w = juce::jlimit (0.0f, 1.0f, smoothedEnv) * track.getWidth();
    auto fill = track.withWidth (w);

    g.setColour (BNT::yellow);
    g.fillRect (fill);

    // LED glow — translucent border when above noise floor.
    if (smoothedEnv > 0.05f)
    {
        g.setColour (BNT::yellow.withAlpha (0.3f));
        g.drawRect (fill.expanded (2.0f), 2.0f);
    }
}
