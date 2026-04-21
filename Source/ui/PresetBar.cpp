#include "PresetBar.h"
#include "LookAndFeel.h"
#include "../PluginProcessor.h"
#include "../presets/Factory.h"

PresetBar::PresetBar (ThomAndGuyAudioProcessor& p) : processor (p)
{
    addAndMakeVisible (dropdown);
    dropdown.setTextWhenNothingSelected ("Preset...");
    dropdown.addItemList (FactoryPresets::listNames(), 1);
    dropdown.onChange = [this]
    {
        const int selected = dropdown.getSelectedItemIndex();
        if (selected >= 0)
            FactoryPresets::load (selected, processor.apvts);
    };
}

PresetBar::~PresetBar() = default;

void PresetBar::paint (juce::Graphics&) {}

void PresetBar::resized()
{
    dropdown.setBounds (getLocalBounds());
}
