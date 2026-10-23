#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs {
    inline constexpr const char* DRIVE        = "Drive";
    inline constexpr const char* CLIP         = "Clip";
    inline constexpr const char* PRE_EMPH     = "PreEmphasis";
    inline constexpr const char* BIT_DEPTH    = "BitDepth";
    inline constexpr const char* SAMPLE_RATE  = "SampleRate";
    inline constexpr const char* SMEAR        = "Smear";
    inline constexpr const char* SPEC_CRUSH   = "SpectralCrush";
    inline constexpr const char* POST_CUTOFF  = "PostCutoff";
    inline constexpr const char* MIX          = "Mix";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Drive: 0-24 dB, linear, default 0 (no gain)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::DRIVE, 1 },
        "Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
        0.0f));

    // Clip: 0-1, linear, default 0 (no clipping)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::CLIP, 1 },
        "Clip",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // PreEmphasis: 0-1, linear, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::PRE_EMPH, 1 },
        "Pre-Emphasis",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // BitDepth: 4-24, integer steps via float param (SliderAttachment requires float), default 24 (clean)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::BIT_DEPTH, 1 },
        "Bit Depth",
        juce::NormalisableRange<float>(4.0f, 24.0f, 1.0f),
        24.0f));

    // SampleRate: 4000-44100 Hz, skewed (log-like), default 44100 (clean)
    {
        auto range = juce::NormalisableRange<float>(4000.0f, 44100.0f, 1.0f);
        range.setSkewForCentre(13000.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::SAMPLE_RATE, 1 },
            "Sample Rate",
            range,
            44100.0f));
    }

    // Smear: 0-1, linear, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SMEAR, 1 },
        "Smear",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // SpectralCrush: 0-1, linear, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SPEC_CRUSH, 1 },
        "Spectral Crush",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // PostCutoff: 1000-20000 Hz, skewed (log-like), default 20000 (fully open)
    {
        auto range = juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f);
        range.setSkewForCentre(4500.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::POST_CUTOFF, 1 },
            "Post Cutoff",
            range,
            20000.0f));
    }

    // Mix: 0-1, linear, default 0 (fully dry = no effect)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::MIX, 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    return layout;
}
