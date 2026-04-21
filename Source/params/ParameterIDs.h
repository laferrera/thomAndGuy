#pragma once

#include <JuceHeader.h>

namespace ParamIDs
{
    inline const juce::String inputGain    = "inputGain";
    inline const juce::String sensitivity  = "sensitivity";
    inline const juce::String attack       = "attack";
    inline const juce::String range        = "range";
    inline const juce::String decay        = "decay";
    inline const juce::String drive        = "drive";
    inline const juce::String morph        = "morph";
    inline const juce::String subBlend     = "subBlend";
    inline const juce::String filterMode   = "filterMode";
    inline const juce::String resonance    = "resonance";
    inline const juce::String baseCutoff   = "baseCutoff";
    inline const juce::String envAmount    = "envAmount";
    inline const juce::String filterType   = "filterType";
    inline const juce::String vowelA       = "vowelA";
    inline const juce::String vowelB       = "vowelB";
    inline const juce::String stretchCurve = "stretchCurve";
    inline const juce::String formantDepth = "formantDepth";
    inline const juce::String wetDry       = "wetDry";
    inline const juce::String outputLevel  = "outputLevel";

    inline const juce::StringArray filterModeChoices   { "Envelope", "Formant" };
    inline const juce::StringArray filterTypeChoices   { "LP", "BP" };
    inline const juce::StringArray vowelChoices        { "AH", "EH", "IH", "OH", "OO" };
    inline const juce::StringArray stretchCurveChoices { "Exp", "Linear", "Log" };

    // Typed views of the choice indices above. Order must match the StringArrays.
    enum class FilterMode : int { Envelope = 0, Formant = 1 };
    enum class FilterType : int { LowPass  = 0, BandPass = 1 };
    enum class StretchCurve : int { Exp = 0, Linear = 1, Log = 2 };
}
