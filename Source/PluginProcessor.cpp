#include "PluginProcessor.h"
#include "PluginEditor.h"

ThomAndGuyAudioProcessor::ThomAndGuyAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void ThomAndGuyAudioProcessor::prepareToPlay (double, int) {}
void ThomAndGuyAudioProcessor::releaseResources() {}

bool ThomAndGuyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void ThomAndGuyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* ThomAndGuyAudioProcessor::createEditor()
{
    return new ThomAndGuyAudioProcessorEditor (*this);
}

void ThomAndGuyAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void ThomAndGuyAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThomAndGuyAudioProcessor();
}
