#pragma once

#include <JuceHeader.h>
#include "params/ParameterLayout.h"
#include "dsp/InputConditioner.h"
#include "dsp/EnvelopeFollower.h"
#include "dsp/WaveshaperChain.h"
#include "dsp/SvfFilter.h"
#include "dsp/FormantBank.h"
#include "params/ParameterIDs.h"

class ThomAndGuyAudioProcessor : public juce::AudioProcessor
{
public:
    ThomAndGuyAudioProcessor();
    ~ThomAndGuyAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Thom & Guy"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getEnvelopeForUI() const noexcept { return envelopeForUI.load(); }

private:
    InputConditioner  inputConditioner;
    EnvelopeFollower  envelopeFollower;
    WaveshaperChain   waveshaperChain;
    SvfFilter         envelopeFilter;
    FormantBank       formantBank;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    static constexpr int oversampleFactor = 2; // log2(4) = 2 => 4x

    std::atomic<float> envelopeForUI { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThomAndGuyAudioProcessor)
};
