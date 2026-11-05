#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
MertsVocalEnhancerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addPercent = [&layout] (const char* id, const char* name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    };

    addPercent ("clean",    "Clean");
    addPercent ("comp",     "Comp");
    addPercent ("presence", "Presence");
    addPercent ("air",      "Air");
    addPercent ("warmth",   "Warmth");

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    return layout;
}

MertsVocalEnhancerAudioProcessor::MertsVocalEnhancerAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    cleanParam    = apvts.getRawParameterValue ("clean");
    compParam     = apvts.getRawParameterValue ("comp");
    presenceParam = apvts.getRawParameterValue ("presence");
    airParam      = apvts.getRawParameterValue ("air");
    warmthParam   = apvts.getRawParameterValue ("warmth");
    outputParam   = apvts.getRawParameterValue ("output");
}

bool MertsVocalEnhancerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void MertsVocalEnhancerAudioProcessor::updateFilters (float clean, float presence, float air)
{
    const auto sr = currentSampleRate;

    if (clean != lastClean)
    {
        lastClean = clean;
        // Clean: high-pass 20 Hz (kapali gibi) -> 140 Hz, 300 Hz civarinda 0 -> -5 dB "camur" kesimi
        *hpf.state    = *Coeffs::makeHighPass (sr, 20.0f + clean * 120.0f, 0.707f);
        *mudCut.state = *Coeffs::makePeakFilter (sr, 300.0f, 1.0f,
                                                 juce::Decibels::decibelsToGain (-5.0f * clean));
    }

    if (presence != lastPresence)
    {
        lastPresence = presence;
        // Presence: 3.5 kHz civari 0 -> +6 dB
        *presenceFilter.state = *Coeffs::makePeakFilter (sr, 3500.0f, 0.9f,
                                                         juce::Decibels::decibelsToGain (6.0f * presence));
    }

    if (air != lastAir)
    {
        lastAir = air;
        // Air: 10 kHz ustu high-shelf 0 -> +6 dB
        *airFilter.state = *Coeffs::makeHighShelf (sr, 10000.0f, 0.707f,
                                                   juce::Decibels::decibelsToGain (6.0f * air));
    }
}

void MertsVocalEnhancerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };

    hpf.prepare (spec);
    mudCut.prepare (spec);
    presenceFilter.prepare (spec);
    airFilter.prepare (spec);
    compressor.prepare (spec);

    compressor.setAttack (15.0f);
    compressor.setRelease (120.0f);

    lastClean = lastPresence = lastAir = -1.0f;   // filtreleri zorla guncelle
    updateFilters (cleanParam->load() * 0.01f,
                   presenceParam->load() * 0.01f,
                   airParam->load() * 0.01f);

    lastMakeupGain = juce::Decibels::decibelsToGain (compParam->load() * 0.01f * 7.0f);
    lastOutputGain = juce::Decibels::decibelsToGain (outputParam->load());
}

void MertsVocalEnhancerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalIn  = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, numSamples);

    const float clean    = cleanParam->load()    * 0.01f;
    const float comp     = compParam->load()     * 0.01f;
    const float presence = presenceParam->load() * 0.01f;
    const float air      = airParam->load()      * 0.01f;
    const float warmth   = warmthParam->load()   * 0.01f;
    const float outputDb = outputParam->load();

    updateFilters (clean, presence, air);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    // 1) Clean
    hpf.process (context);
    mudCut.process (context);

    // 2) Comp (ratio 1 = etkisiz, knob acildikca threshold dusuyor, ratio artiyor)
    compressor.setThreshold (-6.0f - comp * 24.0f);
    compressor.setRatio (1.0f + comp * 5.0f);
    compressor.process (context);

    const float makeupGain = juce::Decibels::decibelsToGain (comp * 7.0f);
    buffer.applyGainRamp (0, numSamples, lastMakeupGain, makeupGain);
    lastMakeupGain = makeupGain;

    // 3) Presence + 4) Air
    presenceFilter.process (context);
    airFilter.process (context);

    // 5) Warmth (yumusak saturation)
    if (warmth > 0.001f)
    {
        const float drive = 1.0f + warmth * 3.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            for (int n = 0; n < numSamples; ++n)
            {
                const float x   = data[n];
                const float wet = std::tanh (drive * x) / drive;
                data[n] = x + warmth * (wet - x);
            }
        }
    }

    // 6) Output
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);
    buffer.applyGainRamp (0, numSamples, lastOutputGain, outputGain);
    lastOutputGain = outputGain;
}

juce::AudioProcessorEditor* MertsVocalEnhancerAudioProcessor::createEditor()
{
    return new MertsVocalEnhancerAudioProcessorEditor (*this);
}

void MertsVocalEnhancerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MertsVocalEnhancerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MertsVocalEnhancerAudioProcessor();
}
