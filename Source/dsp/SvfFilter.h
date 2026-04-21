#pragma once

#include <JuceHeader.h>
#include "Filter.h"

class SvfFilter : public Filter
{
public:
    void prepare      (double sampleRate) override;
    void setMode      (Mode m) override;
    void setCutoffHz  (float cutoff) override;
    void setResonance (float q) override;
    float process     (float input) override;
    void reset()      override;

private:
    juce::dsp::StateVariableTPTFilter<float> svf;
    Mode mode = Mode::LowPass;
};
