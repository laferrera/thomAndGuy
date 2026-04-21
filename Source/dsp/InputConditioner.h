#pragma once

#include <JuceHeader.h>

class InputConditioner
{
public:
    void prepare (double sampleRate);
    void setInputGainDb (float db);
    float process (float input);

private:
    float hpAlpha = 0.0f;
    float prevInput = 0.0f;
    float prevOutput = 0.0f;
    float linearGain = 1.0f;
};
