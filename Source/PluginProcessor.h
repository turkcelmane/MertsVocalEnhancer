#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MertsVocalEnhancerAudioProcessor : public juce::AudioProcessor
{
public:
    MertsVocalEnhancerAudioProcessor();
    ~MertsVocalEnhancerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFilters (float clean, float presence, float air);

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    StereoFilter hpf, mudCut, presenceFilter, airFilter;
    juce::dsp::Compressor<float> compressor;

    std::atomic<float>* cleanParam    = nullptr;
    std::atomic<float>* compParam     = nullptr;
    std::atomic<float>* presenceParam = nullptr;
    std::atomic<float>* airParam      = nullptr;
    std::atomic<float>* warmthParam   = nullptr;
    std::atomic<float>* outputParam   = nullptr;

    double currentSampleRate = 44100.0;
    float lastClean = -1.0f, lastPresence = -1.0f, lastAir = -1.0f;
    float lastMakeupGain = 1.0f, lastOutputGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MertsVocalEnhancerAudioProcessor)
};
