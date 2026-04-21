#include "SvfFilter.h"

void SvfFilter::prepare (double sampleRate)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 1;
    spec.numChannels      = 1;
    svf.prepare (spec);
    setMode (mode);
    svf.setResonance (0.7f);
}

void SvfFilter::setMode (Mode m)
{
    mode = m;
    svf.setType (m == Mode::LowPass
                 ? juce::dsp::StateVariableTPTFilterType::lowpass
                 : juce::dsp::StateVariableTPTFilterType::bandpass);
}

void SvfFilter::setCutoffHz (float cutoff)
{
    svf.setCutoffFrequency (juce::jlimit (10.0f, 20000.0f, cutoff));
}

void SvfFilter::setResonance (float q)
{
    svf.setResonance (juce::jlimit (0.1f, 10.0f, q));
}

float SvfFilter::process (float input)
{
    return svf.processSample (0, input);
}

void SvfFilter::reset()
{
    svf.reset();
}
