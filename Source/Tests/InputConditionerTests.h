#pragma once

#include <JuceHeader.h>
#include "../dsp/InputConditioner.h"

class InputConditionerTests : public juce::UnitTest
{
public:
    InputConditionerTests() : juce::UnitTest ("InputConditioner", "DSP") {}

    void runTest() override
    {
        {
            beginTest ("DC input is removed");
            InputConditioner ic;
            ic.prepare (44100.0);
            ic.setInputGainDb (0.0f);

            float last = 0.0f;
            for (int i = 0; i < 44100; ++i)
                last = ic.process (1.0f);
            expect (std::abs (last) < 0.01f, "DC output should decay below 0.01");
        }

        {
            beginTest ("Unity gain at 0 dB after DC transient");
            InputConditioner ic;
            ic.prepare (44100.0);
            ic.setInputGainDb (0.0f);

            const float freq = 1000.0f;
            const float twoPiF = juce::MathConstants<float>::twoPi * freq;
            float peak = 0.0f;
            for (int i = 0; i < 2048; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / 44100.0f);
                const float y = ic.process (x);
                if (i > 1024) peak = std::max (peak, std::abs (y));
            }
            expect (peak > 0.95f && peak < 1.05f,
                    "Peak of processed 1 kHz sine at 0 dB should be near unity");
        }

        {
            beginTest ("+12 dB gain roughly quadruples amplitude");
            InputConditioner ic;
            ic.prepare (44100.0);
            ic.setInputGainDb (12.0f);

            const float freq = 1000.0f;
            const float twoPiF = juce::MathConstants<float>::twoPi * freq;
            float peak = 0.0f;
            for (int i = 0; i < 4096; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / 44100.0f);
                const float y = ic.process (x);
                if (i > 2048) peak = std::max (peak, std::abs (y));
            }
            expect (peak > 3.7f && peak < 4.2f,
                    "Peak at +12 dB should be near 4x unity sine");
        }
    }
};

static InputConditionerTests inputConditionerTests;
