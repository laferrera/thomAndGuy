#include "WaveshaperChain.h"

void WaveshaperChain::prepare (double sampleRate)
{
    sr = sampleRate;
    sqState = 0.0f;
    const double samples = 4.0;
    const double tau = samples / sampleRate;
    sqSlewCoeff = (float) (1.0 - std::exp (-1.0 / (tau * sampleRate)));
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

    const float blended = (1.0f - morph) * soft + morph * sqState;

    juce::ignoreUnused (subBlend);
    return blended;
}
