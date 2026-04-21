#pragma once

class Filter
{
public:
    enum class Mode { LowPass, BandPass };

    virtual ~Filter() = default;

    virtual void prepare      (double sampleRate) = 0;
    virtual void setMode      (Mode m) = 0;
    virtual void setCutoffHz  (float cutoff) = 0;
    virtual void setResonance (float q) = 0;
    virtual float process     (float input) = 0;
    virtual void reset()      = 0;
};
