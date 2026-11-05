#include "PluginEditor.h"

MertsVocalEnhancerAudioProcessorEditor::MertsVocalEnhancerAudioProcessorEditor (MertsVocalEnhancerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addKnob (clean,    "clean",    "Clean",    " %");
    addKnob (comp,     "comp",     "Comp",     " %");
    addKnob (presence, "presence", "Presence", " %");
    addKnob (air,      "air",      "Air",      " %");
    addKnob (warmth,   "warmth",   "Warmth",   " %");
    addKnob (output,   "output",   "Output",   " dB");

    setSize (720, 360);
}

void MertsVocalEnhancerAudioProcessorEditor::addKnob (Knob& knob, const juce::String& paramId,
                                                      const juce::String& text, const juce::String& suffix)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 22);
    knob.slider.setTextValueSuffix (suffix);
    addAndMakeVisible (knob.slider);

    knob.label.setText (text, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, paramId, knob.slider);
}

void MertsVocalEnhancerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Gecici arka plan: gokyuzu mavisi + gri baseplate seridi (Roblox temasi sonra gelecek)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9fd4ff), 0.0f, 0.0f,
                                             juce::Colour (0xff3f8fe0), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colour (0xff7a7f85));
    g.fillRect (0, getHeight() - 30, getWidth(), 30);

    g.setColour (juce::Colours::white);
    g.setFont (26.0f);
    g.drawText ("Mert's Ultimate Vocal Enchancer v1",
                getLocalBounds().removeFromTop (60), juce::Justification::centred);
}

void MertsVocalEnhancerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50);      // baslik
    area.removeFromBottom (20);   // baseplate seridi

    const int colWidth = area.getWidth() / 6;

    for (auto* k : { &clean, &comp, &presence, &air, &warmth, &output })
    {
        auto col = area.removeFromLeft (colWidth);
        k->label.setBounds (col.removeFromTop (24));
        k->slider.setBounds (col);
    }
}
