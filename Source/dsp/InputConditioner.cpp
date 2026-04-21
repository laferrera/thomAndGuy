#include "InputConditioner.h"

void InputConditioner::prepare (double sampleRate)
{
    const float fc = 20.0f;
    const float rc = 1.0f / (juce::MathConstants<float>::twoPi * fc);
    const float dt = 1.0f / (float) sampleRate;
    hpAlpha = rc / (rc + dt);

    prevInput = 0.0f;
    prevOutput = 0.0f;
}

void InputConditioner::setInputGainDb (float db)
{
    linearGain = juce::Decibels::decibelsToGain (db);
}

float InputConditioner::process (float input)
{
    const float hpOutput = hpAlpha * (prevOutput + input - prevInput);
    prevInput  = input;
    prevOutput = hpOutput;
    return hpOutput * linearGain;
}
