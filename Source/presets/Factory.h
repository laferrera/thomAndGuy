#pragma once

#include <JuceHeader.h>

namespace FactoryPresets
{
    juce::StringArray listNames();
    bool load (int index, juce::AudioProcessorValueTreeState& apvts);
}
