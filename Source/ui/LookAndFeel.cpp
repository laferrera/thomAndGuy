#include "LookAndFeel.h"
#include "BinaryData.h"

ThomAndGuyLookAndFeel::ThomAndGuyLookAndFeel()
{
    romRegularTf  = juce::Typeface::createSystemTypefaceFor (BinaryData::ABCROMRegularTrial_otf,
                                                              BinaryData::ABCROMRegularTrial_otfSize);
    romBoldTf     = juce::Typeface::createSystemTypefaceFor (BinaryData::ABCROMBoldTrial_otf,
                                                              BinaryData::ABCROMBoldTrial_otfSize);
    monoRegularTf = juce::Typeface::createSystemTypefaceFor (BinaryData::ABCROMMonoRegularTrial_otf,
                                                              BinaryData::ABCROMMonoRegularTrial_otfSize);

    setColour (juce::ResizableWindow::backgroundColourId, BNT::black0);

    setColour (juce::Slider::rotarySliderFillColourId,    BNT::cyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, BNT::black2);
    setColour (juce::Slider::thumbColourId,               BNT::cream);
    setColour (juce::Slider::textBoxTextColourId,         BNT::cream);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,    BNT::cyan.withAlpha (0.3f));

    setColour (juce::Label::textColourId,       BNT::nodeGrey);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, BNT::black3);
    setColour (juce::ComboBox::textColourId,       BNT::cream);
    setColour (juce::ComboBox::outlineColourId,    BNT::black2);
    setColour (juce::ComboBox::arrowColourId,      BNT::nodeGrey);
    setColour (juce::ComboBox::focusedOutlineColourId, BNT::cyan);

    setColour (juce::PopupMenu::backgroundColourId,            BNT::black1);
    setColour (juce::PopupMenu::textColourId,                  BNT::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, BNT::magenta);
    setColour (juce::PopupMenu::highlightedTextColourId,       BNT::cream);

    setColour (juce::MidiKeyboardComponent::whiteNoteColourId,          BNT::black2);
    setColour (juce::MidiKeyboardComponent::blackNoteColourId,          BNT::black1);
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,   BNT::black3);
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, BNT::cyan.withAlpha (0.15f));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,     BNT::magenta.withAlpha (0.6f));
    setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, BNT::black1);
    setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,  BNT::nodeGrey);
    setColour (juce::MidiKeyboardComponent::shadowColourId,             juce::Colours::transparentBlack);

    // Slider text boxes use TextEditor internally.
    setColour (juce::TextEditor::backgroundColourId,   BNT::black3);
    setColour (juce::TextEditor::textColourId,         BNT::cream);
    setColour (juce::TextEditor::outlineColourId,      BNT::black2);
    setColour (juce::TextEditor::focusedOutlineColourId, BNT::cyan);
    setColour (juce::TextEditor::highlightColourId,    BNT::cyan.withAlpha (0.3f));

    setColour (juce::CaretComponent::caretColourId, BNT::cyan);
}

//==============================================================================
juce::Font ThomAndGuyLookAndFeel::getMonoFont (float height) const
{
    return juce::Font (juce::FontOptions (monoRegularTf).withHeight (height));
}

juce::Font ThomAndGuyLookAndFeel::getRomFont (float height, bool bold) const
{
    auto tf = bold ? romBoldTf : romRegularTf;
    return juce::Font (juce::FontOptions (tf).withHeight (height));
}

//==============================================================================
void ThomAndGuyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                              int width, int height,
                                              float sliderPos,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& slider)
{
    const float radius  = static_cast<float> (juce::jmin (width, height)) * 0.38f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const float trackWidth = 3.0f;

    // Track background (full arc)
    {
        juce::Path track;
        track.addArc (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (BNT::black2);
        g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Filled arc (value)
    if (sliderPos > 0.001f)
    {
        juce::Path fill;
        fill.addArc (rx, ry, rw, rw, rotaryStartAngle, angle, true);
        g.setColour (BNT::cyan);
        g.strokePath (fill, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Thumb indicator
    {
        const float thumbSize = 5.0f;
        const float thumbRadius = radius + 1.0f;
        float thumbX = centreX + thumbRadius * std::cos (angle - juce::MathConstants<float>::halfPi);
        float thumbY = centreY + thumbRadius * std::sin (angle - juce::MathConstants<float>::halfPi);

        g.setColour (BNT::cream);
        g.fillRect (thumbX - thumbSize * 0.5f, thumbY - thumbSize * 0.5f, thumbSize, thumbSize);
    }

    // Centre dot
    {
        const float dotSize = 3.0f;
        g.setColour (BNT::nodeGrey);
        g.fillRect (centreX - dotSize * 0.5f, centreY - dotSize * 0.5f, dotSize, dotSize);
    }
}

//==============================================================================
void ThomAndGuyLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                          bool /*isButtonDown*/,
                                          int /*buttonX*/, int /*buttonY*/,
                                          int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height);

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRect (bounds);

    auto outlineColour = box.hasKeyboardFocus (false)
                         ? BNT::cyan
                         : box.findColour (juce::ComboBox::outlineColourId);
    g.setColour (outlineColour);
    g.drawRect (bounds, 1);

    // Arrow
    auto arrowZone = juce::Rectangle<int> (width - 20, 0, 20, height);
    juce::Path arrow;
    arrow.addTriangle (static_cast<float> (arrowZone.getX()) + 5.0f, static_cast<float> (arrowZone.getCentreY()) - 2.0f,
                       static_cast<float> (arrowZone.getRight()) - 5.0f, static_cast<float> (arrowZone.getCentreY()) - 2.0f,
                       static_cast<float> (arrowZone.getCentreX()), static_cast<float> (arrowZone.getCentreY()) + 3.0f);
    g.setColour (BNT::nodeGrey);
    g.fillPath (arrow);
}

//==============================================================================
void ThomAndGuyLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (BNT::black1);
    g.setColour (BNT::black3);
    g.drawRect (0, 0, width, height, 1);
}

void ThomAndGuyLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                               bool isSeparator, bool isActive, bool isHighlighted,
                                               bool isTicked, bool /*hasSubMenu*/,
                                               const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                               const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        auto sep = area.reduced (4, 0).withHeight (1).withCentre (area.getCentre());
        g.setColour (BNT::black3);
        g.fillRect (sep);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (BNT::magenta);
        g.fillRect (area);
    }

    g.setColour (isHighlighted ? BNT::cream : (isActive ? BNT::cream : BNT::nodeGrey));
    g.setFont (getPopupMenuFont());
    g.drawText (text, area.reduced (8, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        g.setColour (BNT::cyan);
        auto tickArea = area.withLeft (area.getRight() - area.getHeight()).reduced (6);
        g.fillRect (tickArea.withSizeKeepingCentre (6, 6));
    }
}

//==============================================================================
void ThomAndGuyLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                              bool /*shouldDrawButtonAsHighlighted*/,
                                              bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    float size = 8.0f;
    float cx = bounds.getCentreX() - size * 0.5f;
    float cy = bounds.getCentreY() - size * 0.5f;
    auto box = juce::Rectangle<float> (cx, cy, size, size);

    if (button.getToggleState())
    {
        g.setColour (BNT::cyan);
        g.fillRect (box);
    }
    else
    {
        g.setColour (BNT::nodeGrey);
        g.drawRect (box, 1.0f);
    }
}

//==============================================================================
juce::Font ThomAndGuyLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return getRomFont (13.0f);
}

juce::Font ThomAndGuyLookAndFeel::getPopupMenuFont()
{
    return getRomFont (13.0f);
}

juce::Font ThomAndGuyLookAndFeel::getLabelFont (juce::Label&)
{
    return getMonoFont (10.0f);
}
