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

void ThomAndGuyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1,                                    // mono
        oversampleFactor,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,  // maxOrder
        true); // integerLatency
    oversampler->initProcessing ((size_t) samplesPerBlock);
    oversampler->reset();

    const double osSr = sampleRate * (1 << oversampleFactor);

    inputConditioner.prepare (sampleRate);
    envelopeFollower.prepare (sampleRate);
    waveshaperChain.prepare (osSr);
    envelopeFilter.prepare (sampleRate);
    envelopeFilter.setMode (Filter::Mode::LowPass);
    formantBank.prepare (sampleRate);

    dryBuffer.setSize (1, samplesPerBlock, false, false, false);
    conditionedBuffer.setSize (1, samplesPerBlock, false, false, false);
    shapedBuffer.setSize (1, samplesPerBlock, false, false, false);
    envBuffer.assign ((size_t) samplesPerBlock, 0.0f);

    setLatencySamples ((int) oversampler->getLatencyInSamples());
}

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

    const auto numSamples = buffer.getNumSamples();
    const auto totalIn  = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();

    const float inputGainDb  = *apvts.getRawParameterValue (ParamIDs::inputGain);
    const float sensDb       = *apvts.getRawParameterValue (ParamIDs::sensitivity);
    const float attackMs     = *apvts.getRawParameterValue (ParamIDs::attack);
    const float decayMs      = *apvts.getRawParameterValue (ParamIDs::decay);
    const float range        = *apvts.getRawParameterValue (ParamIDs::range);
    const float driveDb      = *apvts.getRawParameterValue (ParamIDs::drive);
    const float morph        = *apvts.getRawParameterValue (ParamIDs::morph);
    const float subBlend     = *apvts.getRawParameterValue (ParamIDs::subBlend);

    const int   filterModeIx = (int) *apvts.getRawParameterValue (ParamIDs::filterMode);
    const float resonance    = *apvts.getRawParameterValue (ParamIDs::resonance);
    const float baseCutoff   = *apvts.getRawParameterValue (ParamIDs::baseCutoff);
    const float envAmount    = *apvts.getRawParameterValue (ParamIDs::envAmount);
    const int   filterTypeIx = (int) *apvts.getRawParameterValue (ParamIDs::filterType);

    const int   vowelAIx     = (int) *apvts.getRawParameterValue (ParamIDs::vowelA);
    const int   vowelBIx     = (int) *apvts.getRawParameterValue (ParamIDs::vowelB);
    const int   stretchIx    = (int) *apvts.getRawParameterValue (ParamIDs::stretchCurve);
    const float formantDepth = *apvts.getRawParameterValue (ParamIDs::formantDepth);

    const float wetDry       = *apvts.getRawParameterValue (ParamIDs::wetDry);
    const float outLevelDb   = *apvts.getRawParameterValue (ParamIDs::outputLevel);

    inputConditioner.setInputGainDb (inputGainDb);
    envelopeFollower.setSensitivityDb (sensDb);
    envelopeFollower.setAttackMs (attackMs);
    envelopeFollower.setDecayMs (decayMs);
    envelopeFollower.setRange (range);
    waveshaperChain.setDriveDb (driveDb);
    waveshaperChain.setMorph (morph);
    waveshaperChain.setSubBlend (subBlend);
    envelopeFilter.setMode (filterTypeIx == 0 ? Filter::Mode::LowPass : Filter::Mode::BandPass);
    envelopeFilter.setResonance (resonance);
    formantBank.setVowelA (vowelAIx);
    formantBank.setVowelB (vowelBIx);
    formantBank.setResonance (resonance);

    const float outLevelLinear = juce::Decibels::decibelsToGain (outLevelDb);
    const bool  formantActive  = (filterModeIx == 1);

    auto applyStretch = [] (float e, int ix) -> float
    {
        if (ix == 0) return e * e;
        if (ix == 2) return std::sqrt (e);
        return e;
    };

    // Scratch buffer resize guard (host may exceed prepareToPlay's samplesPerBlock).
    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (1, numSamples, false, false, true);
    if (conditionedBuffer.getNumSamples() < numSamples)
        conditionedBuffer.setSize (1, numSamples, false, false, true);
    if (shapedBuffer.getNumSamples() < numSamples)
        shapedBuffer.setSize (1, numSamples, false, false, true);
    if ((int) envBuffer.size() < numSamples)
        envBuffer.resize ((size_t) numSamples);

    float* dry  = dryBuffer.getWritePointer (0);
    float* cond = conditionedBuffer.getWritePointer (0);

    // Pass A — per-sample: collapse to mono, condition, compute env.
    for (int n = 0; n < numSamples; ++n)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < totalIn; ++ch)
            mono += buffer.getReadPointer (ch)[n];
        if (totalIn > 0) mono /= (float) totalIn;

        dry[n]  = mono;
        cond[n] = inputConditioner.process (mono);
        envBuffer[(size_t) n] = envelopeFollower.process (cond[n]);
    }

    // Pass B — block-level: upsample conditioned → waveshape → downsample into shapedBuffer.
    {
        juce::dsp::AudioBlock<float> inBlock (conditionedBuffer.getArrayOfWritePointers(),
                                              1, (size_t) numSamples);
        auto upBlock = oversampler->processSamplesUp (inBlock);

        const size_t upSamples = upBlock.getNumSamples();
        auto* upData = upBlock.getChannelPointer (0);
        for (size_t u = 0; u < upSamples; ++u)
            upData[u] = waveshaperChain.process (upData[u]);

        juce::dsp::AudioBlock<float> outBlock (shapedBuffer.getArrayOfWritePointers(),
                                               1, (size_t) numSamples);
        oversampler->processSamplesDown (outBlock);
    }

    const float* shaped = shapedBuffer.getReadPointer (0);

    // Pass C — per-sample: filter + wet/dry + write to every output channel.
    for (int n = 0; n < numSamples; ++n)
    {
        const float env = envBuffer[(size_t) n];

        float wet = 0.0f;
        if (! formantActive)
        {
            const float cutoff = baseCutoff * std::pow (2.0f, envAmount * env);
            envelopeFilter.setCutoffHz (cutoff);
            wet = envelopeFilter.process (shaped[n]);
        }
        else
        {
            const float morphAmt = formantDepth * applyStretch (env, stretchIx);
            formantBank.setMorph (morphAmt);
            wet = formantBank.process (shaped[n]);
        }

        const float mixed = (wetDry * wet + (1.0f - wetDry) * dry[n]) * outLevelLinear;

        for (int ch = 0; ch < totalOut; ++ch)
            buffer.getWritePointer (ch)[n] = mixed;
    }

    if (numSamples > 0)
        envelopeForUI.store (envBuffer[(size_t) (numSamples - 1)],
                             std::memory_order_relaxed);
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
