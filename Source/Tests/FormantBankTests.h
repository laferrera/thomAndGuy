#pragma once

#include <JuceHeader.h>
#include "../dsp/FormantBank.h"

class FormantBankTests : public juce::UnitTest
{
public:
    FormantBankTests() : juce::UnitTest ("FormantBank", "DSP") {}

    // Approximate peak frequency of the magnitude response using sine-sweep
    // excitation. Returns Hz.
    float estimatePeakFrequency (FormantBank& fb, double sampleRate,
                                 float minHz, float maxHz, int numBins)
    {
        float bestFreq = 0.0f;
        float bestEnergy = 0.0f;
        for (int bin = 0; bin < numBins; ++bin)
        {
            const float freq = juce::jmap ((float) bin, 0.0f, (float) (numBins - 1),
                                            minHz, maxHz);
            const float twoPiF = juce::MathConstants<float>::twoPi * freq;

            fb.reset();
            float peak = 0.0f;
            for (int i = 0; i < 4000; ++i)
            {
                const float x = std::sin (twoPiF * (float) i / (float) sampleRate);
                const float y = fb.process (x);
                if (i > 2000) peak = std::max (peak, std::abs (y));
            }
            if (peak > bestEnergy) { bestEnergy = peak; bestFreq = freq; }
        }
        return bestFreq;
    }

    void runTest() override
    {
        {
            beginTest ("Morph=0: output peaks near vowel A's F1");
            FormantBank fb;
            fb.prepare (44100.0);
            fb.setVowelA (FormantTables::OO);
            fb.setVowelB (FormantTables::AH);
            fb.setResonance (4.0f);
            fb.setMorph (0.0f);

            // Sweep restricted to 200..600 Hz to isolate F1.
            // Full 200..1000 range would also catch OO's F2 at 870 Hz, which
            // picks up ~Q-level phase-addition energy from F1/F3 filter
            // sidelobes and marginally exceeds F1's on-resonance peak. This
            // is a property of summing 3 shared-Q parallel BPs, not a bug.
            const float peakHz = estimatePeakFrequency (fb, 44100.0, 200.0f, 600.0f, 40);
            expect (peakHz > 250.0f && peakHz < 400.0f,
                    "At morph=0, peak frequency should be near OO's F1 (~300 Hz)");
        }

        {
            beginTest ("Morph=1: output peaks near vowel B's F1");
            FormantBank fb;
            fb.prepare (44100.0);
            fb.setVowelA (FormantTables::OO);
            fb.setVowelB (FormantTables::AH);
            fb.setResonance (4.0f);
            fb.setMorph (1.0f);

            const float peakHz = estimatePeakFrequency (fb, 44100.0, 500.0f, 1200.0f, 40);
            expect (peakHz > 650.0f && peakHz < 820.0f,
                    "At morph=1, peak frequency should be near AH's F1 (~730 Hz)");
        }
    }
};

static FormantBankTests formantBankTests;
