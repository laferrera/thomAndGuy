#pragma once

#include <JuceHeader.h>
#include <array>
#include "SvfFilter.h"
#include "FormantTables.h"

class FormantBank
{
public:
    void prepare       (double sampleRate);
    void reset();

    void setVowelA    (int vowelIndex);
    void setVowelB    (int vowelIndex);
    void setMorph     (float value);    // 0..1, A -> B crossfade
    void setResonance (float q);

    float process (float input);

private:
    std::array<SvfFilter, 3> bankA;
    std::array<SvfFilter, 3> bankB;
    float morph = 0.0f;
    float q = 4.0f;

    static void applyVowel (std::array<SvfFilter, 3>& bank, int vowelIndex, float q);
};
