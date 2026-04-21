#pragma once

#include <JuceHeader.h>

namespace ParamSmoothers
{
    // Typical choice for parameter smoothing. Long enough to avoid zipper
    // noise, short enough that the UI feels responsive.
    constexpr double defaultSmoothingSeconds = 0.020; // 20 ms

    inline void prepare (juce::SmoothedValue<float>& s, double sampleRate,
                         double smoothingSeconds = defaultSmoothingSeconds)
    {
        s.reset (sampleRate, smoothingSeconds);
    }

    // Sets a new target without ramp — use for initial state.
    inline void setImmediate (juce::SmoothedValue<float>& s, float value)
    {
        s.setCurrentAndTargetValue (value);
    }
}
