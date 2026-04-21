#include "EnvelopeFollower.h"

namespace
{
    constexpr float fastReleaseMs    = 15.0f;
    constexpr float slowAttackMs     = 12.0f;
    constexpr float outputSmoothingMs = 2.0f;
}

float EnvelopeFollower::msToCoeff (float ms, double sampleRate)
{
    if (ms <= 0.0f) return 0.0f;
    const double tau = (double) ms * 0.001;
    return (float) (1.0 - std::exp (-1.0 / (tau * sampleRate)));
}

void EnvelopeFollower::prepare (double sampleRate)
{
    sr = sampleRate;
    fastState = slowState = outState = 0.0f;

    fastCoeffR = msToCoeff (fastReleaseMs, sr);
    slowCoeffA = msToCoeff (slowAttackMs, sr);
    outCoeff   = msToCoeff (outputSmoothingMs, sr);

    setAttackMs (4.0f);
    setDecayMs  (300.0f);
}

void EnvelopeFollower::setSensitivityDb (float db)
{
    sensitivityGain = juce::Decibels::decibelsToGain (db);
}

void EnvelopeFollower::setAttackMs (float ms)
{
    fastCoeffA = msToCoeff (ms, sr);
}

void EnvelopeFollower::setDecayMs (float ms)
{
    slowCoeffR = msToCoeff (ms, sr);
}

void EnvelopeFollower::setRange (float value)
{
    range = juce::jlimit (0.0f, 1.0f, value);
}

float EnvelopeFollower::process (float input)
{
    const float rect = std::abs (input) * sensitivityGain;

    {
        const float coeff = (rect > fastState) ? fastCoeffA : fastCoeffR;
        fastState += coeff * (rect - fastState);
    }

    {
        const float coeff = (rect > slowState) ? slowCoeffA : slowCoeffR;
        slowState += coeff * (rect - slowState);
    }

    float env = std::max (fastState, slowState);

    const float shaped = env + range * (std::tanh (env * 1.5f) / std::tanh (1.5f) - env);

    const float clamped = juce::jlimit (0.0f, 1.0f, shaped);
    outState += outCoeff * (clamped - outState);
    return outState;
}
