#include "FormantBank.h"

void FormantBank::prepare (double sampleRate)
{
    for (auto& f : bankA) { f.prepare (sampleRate); f.setMode (Filter::Mode::BandPass); }
    for (auto& f : bankB) { f.prepare (sampleRate); f.setMode (Filter::Mode::BandPass); }
    applyVowel (bankA, FormantTables::OO, q);
    applyVowel (bankB, FormantTables::AH, q);
}

void FormantBank::reset()
{
    for (auto& f : bankA) f.reset();
    for (auto& f : bankB) f.reset();
}

void FormantBank::setVowelA    (int vowelIndex) { applyVowel (bankA, vowelIndex, q); }
void FormantBank::setVowelB    (int vowelIndex) { applyVowel (bankB, vowelIndex, q); }
void FormantBank::setMorph     (float value)    { morph = juce::jlimit (0.0f, 1.0f, value); }

void FormantBank::setResonance (float newQ)
{
    q = newQ;
    for (auto& f : bankA) f.setResonance (q);
    for (auto& f : bankB) f.setResonance (q);
}

void FormantBank::applyVowel (std::array<SvfFilter, 3>& bank, int vowelIndex, float q)
{
    const auto fs = FormantTables::get (vowelIndex);
    bank[0].setCutoffHz (fs.f1);
    bank[1].setCutoffHz (fs.f2);
    bank[2].setCutoffHz (fs.f3);
    for (auto& f : bank) f.setResonance (q);
}

float FormantBank::process (float input)
{
    float a = 0.0f, b = 0.0f;
    for (auto& f : bankA) a += f.process (input);
    for (auto& f : bankB) b += f.process (input);
    a *= (1.0f / 3.0f);
    b *= (1.0f / 3.0f);
    return (1.0f - morph) * a + morph * b;
}
