#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "DegradeLookAndFeel.h"

class DegradeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DegradeAudioProcessorEditor(DegradeAudioProcessor&);
    ~DegradeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    DegradeLookAndFeel degradeLookAndFeel;

    DegradeAudioProcessor& processorRef;

    juce::Slider driveSlider, clipSlider, preEmphSlider, bitDepthSlider,
                 sampleRateSlider, smearSlider, specCrushSlider,
                 postCutoffSlider, mixSlider;

    juce::Label driveLabel, clipLabel, preEmphLabel, bitDepthLabel,
                sampleRateLabel, smearLabel, specCrushLabel,
                postCutoffLabel, mixLabel;

    std::unique_ptr<SliderAttachment> driveAttach, clipAttach, preEmphAttach,
                                       bitDepthAttach, sampleRateAttach,
                                       smearAttach, specCrushAttach,
                                       postCutoffAttach, mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DegradeAudioProcessorEditor)
};
