#pragma once

#include <JuceHeader.h>

class SubOctaveDivider
{
public:
    void prepare (double sampleRate);
    float process (float input);

private:
    float  flipFlop = 0.0f;
    float  lastSignedInput = 0.0f;
    float  lpState = 0.0f;
    float  lpCoeff = 0.0f;
};
