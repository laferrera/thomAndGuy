#include "SubOctaveDivider.h"

void SubOctaveDivider::prepare (double sampleRate)
{
    flipFlop = 0.0f;
    lastSignedInput = 0.0f;
    lpState = 0.0f;

    const float fc = 1000.0f;
    const double tau = 1.0 / (juce::MathConstants<double>::twoPi * fc);
    lpCoeff = (float) (1.0 - std::exp (-1.0 / (tau * sampleRate)));
}

float SubOctaveDivider::process (float input)
{
    if (lastSignedInput <= 0.0f && input > 0.0f)
        flipFlop = (flipFlop > 0.0f ? -1.0f : 1.0f);
    lastSignedInput = input;

    lpState += lpCoeff * (flipFlop - lpState);
    return lpState;
}
