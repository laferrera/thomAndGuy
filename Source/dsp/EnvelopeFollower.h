#pragma once

#include <JuceHeader.h>

class EnvelopeFollower
{
public:
    void prepare (double sampleRate);

    void setSensitivityDb (float db);
    void setAttackMs      (float ms);
    void setDecayMs       (float ms);
    void setRange         (float value);

    // Returns env(t) in [0, 1].
    float process (float input);

private:
    double sr = 44100.0;
    float  sensitivityGain = 1.0f;
    float  range = 0.5f;

    float fastCoeffA = 0.0f;
    float fastCoeffR = 0.0f;
    float fastState  = 0.0f;

    float slowCoeffA = 0.0f;
    float slowCoeffR = 0.0f;
    float slowState  = 0.0f;

    float outState = 0.0f;
    float outCoeff = 0.0f;

    static float msToCoeff (float ms, double sampleRate);
};
