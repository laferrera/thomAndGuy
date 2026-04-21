#include "WaveshaperChain.h"

void WaveshaperChain::prepare (double sampleRate)
{
    sqState = 0.0f;
    const double samples = 4.0;
    const double tau = samples / sampleRate;
    sqSlewCoeff = (float) (1.0 - std::exp (-1.0 / (tau * sampleRate)));

    subDivider.prepare (sampleRate);
}

void WaveshaperChain::setDriveDb  (float db)    { driveGain = juce::Decibels::decibelsToGain (db); }
void WaveshaperChain::setMorph    (float value) { morph    = juce::jlimit (0.0f, 1.0f, value); }
void WaveshaperChain::setSubBlend (float value) { subBlend = juce::jlimit (0.0f, 1.0f, value); }

float WaveshaperChain::process (float input)
{
    const float driven = input * driveGain;

    const float soft = std::tanh (driven);

    const float target = (driven > 0.0f) ? 1.0f : (driven < 0.0f ? -1.0f : 0.0f);
    sqState += sqSlewCoeff * (target - sqState);

    const float shaped = (1.0f - morph) * soft + morph * sqState;

    const float sub = subDivider.process (sqState);

    return (1.0f - subBlend) * shaped + subBlend * sub;
}
