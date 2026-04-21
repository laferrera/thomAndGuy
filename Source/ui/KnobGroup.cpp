#include "KnobGroup.h"
#include "LookAndFeel.h"

KnobGroup::KnobGroup (juce::String title) : groupTitle (std::move (title)) {}

void KnobGroup::addSlider (ValueSlider& slider, juce::String label)
{
    entries.push_back ({ &slider, std::move (label) });
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
}

void KnobGroup::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour (BNT::black2);
    g.fillRect (area);
    g.setColour (BNT::black3);
    g.drawRect (area, 1);

    g.setColour (BNT::cream);
    g.setFont (12.0f);
    const auto titleArea = area.removeFromTop (16).reduced (6, 0);
    g.drawFittedText (groupTitle.toUpperCase(), titleArea,
                       juce::Justification::centredLeft, 1);
}

void KnobGroup::resized()
{
    auto area = getLocalBounds().reduced (6, 4);
    area.removeFromTop (16);
    if (entries.empty()) return;

    const int slotW = area.getWidth() / (int) entries.size();
    for (auto& e : entries)
    {
        auto slot = area.removeFromLeft (slotW);
        e.slider->setBounds (slot.reduced (2));
    }
}
