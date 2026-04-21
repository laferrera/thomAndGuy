/*
  ==============================================================================

    LookAndFeel — && design system for JUCE
    Ported from ../PhysicalModelTest/Source/AmpersandLookAndFeel
    Dark register, CMY accents, sharp corners, ABC ROM fonts

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// NOLINTBEGIN
namespace BNT
{
    inline const juce::Colour black0     (0xff000000);
    inline const juce::Colour black1     (0xff1a1a1a);
    inline const juce::Colour black2     (0xff333333);
    inline const juce::Colour black3     (0xff4d4d4d);
    inline const juce::Colour nodeGrey   (0xff666666);
    inline const juce::Colour cream      (0xfffffffe);
    inline const juce::Colour cyan       (0xff0093d3);
    inline const juce::Colour magenta    (0xffcc016b);
    inline const juce::Colour yellow     (0xfffff10d);
}
// NOLINTEND

class ThomAndGuyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThomAndGuyLookAndFeel();
    ~ThomAndGuyLookAndFeel() override = default;

    juce::Font getMonoFont (float height) const;
    juce::Font getRomFont (float height, bool bold = false) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;

private:
    juce::Typeface::Ptr romRegularTf, romBoldTf, monoRegularTf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThomAndGuyLookAndFeel)
};
