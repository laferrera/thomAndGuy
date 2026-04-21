#pragma once

#include <JuceHeader.h>

// Slider with:
//  - dynamic hover tooltip showing "<Label>: <current value + unit>"
//  - cmd-click to reset to default value (set via setDoubleClickReturnValue)
class ValueSlider : public juce::Slider
{
public:
    void setParamLabel (juce::String label) { paramLabel = std::move (label); }

    juce::String getTooltip() override
    {
        return paramLabel.isEmpty()
                 ? getTextFromValue (getValue())
                 : paramLabel + ": " + getTextFromValue (getValue());
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isCommandDown() && isEnabled() && isDoubleClickReturnEnabled())
        {
            setValue (getDoubleClickReturnValue(), juce::sendNotificationSync);
            return;
        }
        juce::Slider::mouseDown (e);
    }

private:
    juce::String paramLabel;
};
