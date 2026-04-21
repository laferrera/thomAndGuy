#pragma once

#include <JuceHeader.h>
#include "SubOctaveDivider.h"

class WaveshaperChain
{
public:
    void prepare (double sampleRate);

    void setDriveDb  (float db);
    void setMorph    (float value);    // 0 = soft, 1 = hard
    void setSubBlend (float value);    // sub-octave mix, 0..1 (implemented Task 8)

    float process (float input);

private:
    double sr = 44100.0;
    float  driveGain = 1.0f;
    float  morph = 0.0f;
    float  subBlend = 0.0f;

    float sqState = 0.0f;
    float sqSlewCoeff = 0.0f;

    SubOctaveDivider subDivider;
};
