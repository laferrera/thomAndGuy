#include "ParameterLayout.h"
#include "ParameterIDs.h"

namespace
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using NR  = juce::NormalisableRange<float>;

    NR dbRange (float lo, float hi) { return NR (lo, hi, 0.01f); }
    NR logRange (float lo, float hi, float skew)
    {
        NR r (lo, hi, 0.0001f);
        r.setSkewForCentre (lo + (hi - lo) * skew);
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ParameterLayout::create()
{
    using Params = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;
    Params p;

    // Input
    p.push_back (std::make_unique<APF> (ParamIDs::inputGain,   "Input Gain",
                  dbRange (-12.0f, 12.0f), 0.0f, "dB"));

    // Envelope Follower
    p.push_back (std::make_unique<APF> (ParamIDs::sensitivity, "Sensitivity",
                  dbRange (0.0f, 24.0f), 12.0f, "dB"));
    p.push_back (std::make_unique<APF> (ParamIDs::attack,      "Attack",
                  logRange (1.0f, 30.0f, 0.3f), 4.0f, "ms"));
    p.push_back (std::make_unique<APF> (ParamIDs::range,       "Range",
                  NR (0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back (std::make_unique<APF> (ParamIDs::decay,       "Decay",
                  logRange (100.0f, 800.0f, 0.3f), 300.0f, "ms"));

    // Waveshaper
    p.push_back (std::make_unique<APF> (ParamIDs::drive,       "Drive",
                  dbRange (0.0f, 30.0f), 6.0f, "dB"));
    p.push_back (std::make_unique<APF> (ParamIDs::morph,       "Morph",
                  NR (0.0f, 1.0f, 0.001f), 0.6f));
    p.push_back (std::make_unique<APF> (ParamIDs::subBlend,    "Sub Blend",
                  NR (0.0f, 1.0f, 0.001f), 0.3f));

    // Filter (global)
    p.push_back (std::make_unique<APC> (ParamIDs::filterMode,  "Filter Mode",
                  ParamIDs::filterModeChoices, 0));
    p.push_back (std::make_unique<APF> (ParamIDs::resonance,   "Resonance",
                  logRange (0.5f, 12.0f, 0.3f), 3.0f));

    // Envelope Mode
    p.push_back (std::make_unique<APF> (ParamIDs::baseCutoff,  "Base Cutoff",
                  logRange (80.0f, 4000.0f, 0.3f), 250.0f, "Hz"));
    p.push_back (std::make_unique<APF> (ParamIDs::envAmount,   "Env Amount",
                  NR (-4.0f, 4.0f, 0.01f), 2.5f, "oct"));
    p.push_back (std::make_unique<APC> (ParamIDs::filterType,  "Filter Type",
                  ParamIDs::filterTypeChoices, 0));

    // Formant Mode
    p.push_back (std::make_unique<APC> (ParamIDs::vowelA,      "Vowel A",
                  ParamIDs::vowelChoices, 4 /* OO */));
    p.push_back (std::make_unique<APC> (ParamIDs::vowelB,      "Vowel B",
                  ParamIDs::vowelChoices, 0 /* AH */));
    p.push_back (std::make_unique<APC> (ParamIDs::stretchCurve, "Stretch Curve",
                  ParamIDs::stretchCurveChoices, 1 /* Linear */));
    p.push_back (std::make_unique<APF> (ParamIDs::formantDepth, "Formant Depth",
                  NR (0.0f, 1.0f, 0.001f), 1.0f));

    // Output
    p.push_back (std::make_unique<APF> (ParamIDs::wetDry,      "Wet/Dry",
                  NR (0.0f, 1.0f, 0.001f), 1.0f));
    p.push_back (std::make_unique<APF> (ParamIDs::outputLevel, "Output Level",
                  dbRange (-12.0f, 12.0f), 0.0f, "dB"));

    return { p.begin(), p.end() };
}
