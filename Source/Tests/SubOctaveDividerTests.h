#pragma once

#include <JuceHeader.h>
#include "../dsp/SubOctaveDivider.h"

class SubOctaveDividerTests : public juce::UnitTest
{
public:
    SubOctaveDividerTests() : juce::UnitTest ("SubOctaveDivider", "DSP") {}

    void runTest() override
    {
        {
            beginTest ("440 Hz sine produces output at 220 Hz");
            SubOctaveDivider div;
            div.prepare (44100.0);

            const double sr = 44100.0;
            const float f = 440.0f;
            const float twoPiF = juce::MathConstants<float>::twoPi * f;

            int crossings = 0;
            float prev = 0.0f;
            for (int i = 0; i < (int) sr; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / (float) sr);
                const float y = div.process (x);
                if (prev < 0.0f && y >= 0.0f) ++crossings;
                prev = y;
            }
            expect (crossings >= 215 && crossings <= 225,
                    "Output should have ~220 positive-going zero-crossings");
        }

        {
            beginTest ("Silence produces zero output after settling");
            SubOctaveDivider div;
            div.prepare (44100.0);

            for (int i = 0; i < 4410; ++i) div.process (0.0f);
            const float y = div.process (0.0f);
            expect (std::abs (y) < 0.01f, "Sub output should decay to zero");
        }
    }
};

static SubOctaveDividerTests subOctaveDividerTests;
