#pragma once

#include "PluginProcessor.h"

class MertsVocalEnhancerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MertsVocalEnhancerAudioProcessorEditor (MertsVocalEnhancerAudioProcessor&);
    ~MertsVocalEnhancerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob& knob, const juce::String& paramId,
                  const juce::String& text, const juce::String& suffix);

    MertsVocalEnhancerAudioProcessor& audioProcessor;

    Knob clean, comp, presence, air, warmth, output;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MertsVocalEnhancerAudioProcessorEditor)
};
