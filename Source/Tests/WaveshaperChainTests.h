#pragma once

#include <JuceHeader.h>
#include "../dsp/WaveshaperChain.h"

class WaveshaperChainTests : public juce::UnitTest
{
public:
    WaveshaperChainTests() : juce::UnitTest ("WaveshaperChain", "DSP") {}

    void runTest() override
    {
        {
            beginTest ("At morph=0 drive=0, small input is near unity");
            WaveshaperChain ws;
            ws.prepare (44100.0);
            ws.setDriveDb (0.0f);
            ws.setMorph (0.0f);
            ws.setSubBlend (0.0f);

            const float y = ws.process (0.1f);
            expect (std::abs (y - 0.1f) < 0.01f,
                    "Low-amplitude tanh soft-clip should be near linear");
        }

        {
            beginTest ("At morph=1 (hard-square), output polarity matches input sign");
            WaveshaperChain ws;
            ws.prepare (44100.0);
            ws.setDriveDb (0.0f);
            ws.setMorph (1.0f);
            ws.setSubBlend (0.0f);

            for (int i = 0; i < 20; ++i) ws.process (0.5f);
            const float yPos = ws.process (0.5f);
            for (int i = 0; i < 20; ++i) ws.process (-0.5f);
            const float yNeg = ws.process (-0.5f);

            expect (yPos > 0.8f, "Positive input at morph=1 should saturate near +1");
            expect (yNeg < -0.8f, "Negative input at morph=1 should saturate near -1");
        }

        {
            beginTest ("Drive increases soft-clip saturation");
            WaveshaperChain wsLow;  wsLow.prepare (44100.0);
            wsLow.setDriveDb (0.0f); wsLow.setMorph (0.0f); wsLow.setSubBlend (0.0f);

            WaveshaperChain wsHigh; wsHigh.prepare (44100.0);
            wsHigh.setDriveDb (24.0f); wsHigh.setMorph (0.0f); wsHigh.setSubBlend (0.0f);

            const float low  = std::abs (wsLow.process  (0.3f));
            const float high = std::abs (wsHigh.process (0.3f));
            expect (high > 0.9f && high <= 1.0f,
                    "Heavily driven tanh should saturate near 1");
            expect (low < 0.31f,
                    "Undriven tanh on 0.3 input should stay near 0.3");
        }

        {
            beginTest ("Output stays bounded across all morph positions");
            WaveshaperChain ws;
            ws.prepare (44100.0);
            ws.setDriveDb (30.0f);
            ws.setSubBlend (0.0f);

            for (float m = 0.0f; m <= 1.0f; m += 0.1f)
            {
                ws.setMorph (m);
                for (int i = 0; i < 1000; ++i)
                {
                    const float x = 0.8f * std::sin ((float) i * 0.05f);
                    const float y = ws.process (x);
                    expect (std::abs (y) < 1.05f,
                            "Shaper output must stay bounded near |y| <= 1");
                }
            }
        }

        {
            beginTest ("Sub blend non-zero produces sub-octave component");
            WaveshaperChain ws;
            ws.prepare (44100.0);
            ws.setDriveDb  (0.0f);
            ws.setMorph    (1.0f);
            ws.setSubBlend (1.0f);

            const double sr = 44100.0;
            const float twoPiF = juce::MathConstants<float>::twoPi * 440.0f;

            int crossings = 0;
            float prev = 0.0f;
            for (int i = 0; i < (int) sr; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / (float) sr);
                const float y = ws.process (x);
                if (prev < 0.0f && y >= 0.0f) ++crossings;
                prev = y;
            }
            expect (crossings >= 210 && crossings <= 230,
                    "Sub-only output should have roughly 220 zero-crossings");
        }
    }
};

static WaveshaperChainTests waveshaperChainTests;
