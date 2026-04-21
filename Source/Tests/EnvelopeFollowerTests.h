#pragma once

#include <JuceHeader.h>
#include "../dsp/EnvelopeFollower.h"

class EnvelopeFollowerTests : public juce::UnitTest
{
public:
    EnvelopeFollowerTests() : juce::UnitTest ("EnvelopeFollower", "DSP") {}

    void runTest() override
    {
        {
            beginTest ("Silence produces zero envelope");
            EnvelopeFollower env;
            env.prepare (44100.0);
            env.setSensitivityDb (12.0f);
            env.setAttackMs (4.0f);
            env.setDecayMs (300.0f);
            env.setRange (0.5f);

            float out = 1.0f;
            for (int i = 0; i < 4410; ++i)
                out = env.process (0.0f);
            expect (out < 0.01f, "Silence should drain envelope to near zero");
        }

        {
            beginTest ("Step input rises within attack window");
            EnvelopeFollower env;
            env.prepare (44100.0);
            env.setSensitivityDb (12.0f);
            env.setAttackMs (4.0f);
            env.setDecayMs (300.0f);
            env.setRange (0.5f);

            float out = 0.0f;
            const int samples4ms = 4 * 44;
            for (int i = 0; i < samples4ms * 3; ++i)
                out = env.process (0.5f);
            expect (out > 0.3f, "Envelope should rise during sustained input");
        }

        {
            beginTest ("Long decay outlasts short attack (slow detector)");
            EnvelopeFollower env;
            env.prepare (44100.0);
            env.setSensitivityDb (12.0f);
            env.setAttackMs (2.0f);
            env.setDecayMs (500.0f);
            env.setRange (0.5f);

            for (int i = 0; i < 220; ++i)
                env.process (1.0f);

            float out = 1.0f;
            for (int i = 0; i < 4410; ++i)
                out = env.process (0.0f);

            expect (out > 0.2f,
                    "Envelope should sustain after transient due to slow detector");
        }

        {
            beginTest ("Output is bounded to [0, 1]");
            EnvelopeFollower env;
            env.prepare (44100.0);
            env.setSensitivityDb (24.0f);
            env.setAttackMs (1.0f);
            env.setDecayMs (800.0f);
            env.setRange (1.0f);

            float maxOut = 0.0f;
            for (int i = 0; i < 10000; ++i)
            {
                const float x = 2.0f * std::sin ((float) i * 0.1f);
                const float y = env.process (x);
                maxOut = std::max (maxOut, y);
                expect (y >= 0.0f && y <= 1.0f, "Envelope must stay within [0, 1]");
            }
            expect (maxOut > 0.9f, "Max envelope should saturate near 1.0");
        }
    }
};

static EnvelopeFollowerTests envelopeFollowerTests;
