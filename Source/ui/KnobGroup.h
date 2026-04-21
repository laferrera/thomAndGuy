#pragma once

#include <JuceHeader.h>
#include "ValueSlider.h"

class KnobGroup : public juce::Component
{
public:
    explicit KnobGroup (juce::String title);

    // Non-owning — caller keeps the slider alive.
    void addSlider (ValueSlider& slider, juce::String label);

    void paint    (juce::Graphics&) override;
    void resized  ()                override;

private:
    struct Entry { ValueSlider* slider; juce::String label; };

    juce::String groupTitle;
    std::vector<Entry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobGroup)
};
