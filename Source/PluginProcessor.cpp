#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Tests/TestRunner.h"

ThomAndGuyAudioProcessor::ThomAndGuyAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", ParameterLayout::create())
{
    static bool testsAttempted = false;
    if (! testsAttempted)
    {
        testsAttempted = true;
        if (juce::JUCEApplicationBase::isStandaloneApp())
        {
            const auto args = juce::JUCEApplicationBase::getCommandLineParameterArray();
            if (args.contains ("--run-tests"))
            {
                juce::UnitTestRunner runner;
                runner.setAssertOnFailure (false);
                runner.runAllTests();

                int failures = 0;
                for (int i = 0; i < runner.getNumResults(); ++i)
                    if (auto* r = runner.getResult (i))
                        failures += r->failures;

                std::quick_exit (failures == 0 ? 0 : 1);
            }
        }
    }
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

void ThomAndGuyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ThomAndGuyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThomAndGuyAudioProcessor();
}
