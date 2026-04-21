#pragma once

#include <JuceHeader.h>
#include <array>

namespace FormantTables
{
    enum Vowel { AH = 0, EH = 1, IH = 2, OH = 3, OO = 4 };
    constexpr int numVowels = 5;

    struct Formants { float f1, f2, f3; };

    inline constexpr std::array<Formants, numVowels> formants =
    {{
        { 730.0f, 1090.0f, 2440.0f }, // AH  (father)
        { 530.0f, 1840.0f, 2480.0f }, // EH  (bed)
        { 390.0f, 1990.0f, 2550.0f }, // IH  (bit)
        { 570.0f,  840.0f, 2410.0f }, // OH  (boat)
        { 300.0f,  870.0f, 2240.0f }, // OO  (boot)
    }};

    inline Formants get (int vowelIndex)
    {
        return formants[juce::jlimit (0, numVowels - 1, vowelIndex)];
    }
}
