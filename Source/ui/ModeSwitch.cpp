#include "ModeSwitch.h"
#include "LookAndFeel.h"

ModeSwitch::ModeSwitch (juce::AudioProcessorValueTreeState& s, const juce::String& id,
                        juce::StringRef labelA, juce::StringRef labelB)
    : apvts (s), paramID (id),
      valueAttachment (*s.getParameter (id), [this] (float v)
      {
          const int ix = juce::roundToInt (v);
          setSelectedIndex (ix);
          if (onChange) onChange (ix);
      })
{
    buttonA.setButtonText (labelA);
    buttonB.setButtonText (labelB);
    buttonA.setClickingTogglesState (true);
    buttonB.setClickingTogglesState (true);
    buttonA.setRadioGroupId (1);
    buttonB.setRadioGroupId (1);
    buttonA.onClick = [this] { valueAttachment.setValueAsCompleteGesture (0.0f); };
    buttonB.onClick = [this] { valueAttachment.setValueAsCompleteGesture (1.0f); };

    addAndMakeVisible (buttonA);
    addAndMakeVisible (buttonB);

    valueAttachment.sendInitialUpdate();
}

ModeSwitch::~ModeSwitch() = default;

void ModeSwitch::setSelectedIndex (int ix)
{
    buttonA.setToggleState (ix == 0, juce::dontSendNotification);
    buttonB.setToggleState (ix == 1, juce::dontSendNotification);

    const auto setColour = [] (juce::TextButton& b, bool selected)
    {
        b.setColour (juce::TextButton::buttonColourId,
                     selected ? BNT::magenta : juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::textColourOnId,  BNT::cream);
        b.setColour (juce::TextButton::textColourOffId, BNT::nodeGrey);
    };
    setColour (buttonA, ix == 0);
    setColour (buttonB, ix == 1);
}

void ModeSwitch::paint (juce::Graphics& g)
{
    g.setColour (BNT::cream);
    g.setFont (12.0f);
    auto area = getLocalBounds();
    const auto label = area.removeFromLeft (110);
    g.drawFittedText ("FILTER MODE:", label, juce::Justification::centredLeft, 1);
}

void ModeSwitch::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft (110);
    const int w = (area.getWidth() - 8) / 2;
    buttonA.setBounds (area.removeFromLeft (w));
    area.removeFromLeft (8);
    buttonB.setBounds (area.removeFromLeft (w));
}
