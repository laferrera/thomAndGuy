#pragma once

#include <JuceHeader.h>
#include "../dsp/SvfFilter.h"

class SvfFilterTests : public juce::UnitTest
{
public:
    SvfFilterTests() : juce::UnitTest ("SvfFilter", "DSP") {}

    void runTest() override
    {
        {
            beginTest ("LP: 1 kHz sine through LP at 200 Hz is strongly attenuated");
            SvfFilter f;
            f.prepare (44100.0);
            f.setMode (Filter::Mode::LowPass);
            f.setCutoffHz (200.0f);
            f.setResonance (0.7f);

            const float twoPiF = juce::MathConstants<float>::twoPi * 1000.0f;
            float peakIn = 0.0f, peakOut = 0.0f;
            for (int i = 0; i < 4096; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / 44100.0f);
                const float y = f.process (x);
                if (i > 2048)
                {
                    peakIn  = std::max (peakIn,  std::abs (x));
                    peakOut = std::max (peakOut, std::abs (y));
                }
            }
            const float gainDb = juce::Decibels::gainToDecibels (peakOut / peakIn);
            expect (gainDb < -18.0f, "LP at 200 Hz should attenuate 1 kHz by >18 dB");
        }

        {
            beginTest ("BP: center frequency passes at approximately unity");
            SvfFilter f;
            f.prepare (44100.0);
            f.setMode (Filter::Mode::BandPass);
            f.setCutoffHz (1000.0f);
            f.setResonance (2.0f);

            const float twoPiF = juce::MathConstants<float>::twoPi * 1000.0f;
            float peakIn = 0.0f, peakOut = 0.0f;
            for (int i = 0; i < 4096; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / 44100.0f);
                const float y = f.process (x);
                if (i > 2048)
                {
                    peakIn  = std::max (peakIn,  std::abs (x));
                    peakOut = std::max (peakOut, std::abs (y));
                }
            }
            expect (peakOut > 0.5f * peakIn,
                    "BP at center freq should pass substantial energy");
        }

        {
            beginTest ("Filter remains stable at high Q with fast cutoff changes");
            SvfFilter f;
            f.prepare (44100.0);
            f.setMode (Filter::Mode::LowPass);
            f.setResonance (10.0f);

            bool bounded = true;
            for (int i = 0; i < 10000; ++i)
            {
                const float cutoff = 100.0f + 3000.0f * (0.5f + 0.5f * std::sin ((float) i * 0.01f));
                f.setCutoffHz (cutoff);
                const float x = std::sin ((float) i * 0.05f);
                const float y = f.process (x);
                if (! std::isfinite (y) || std::abs (y) > 50.0f) { bounded = false; break; }
            }
            expect (bounded, "Filter should stay bounded under fast cutoff modulation");
        }
    }
};

static SvfFilterTests svfFilterTests;
