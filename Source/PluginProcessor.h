#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "DSP.h"

class DegradeAudioProcessor : public juce::AudioProcessor
{
public:
    DegradeAudioProcessor();
    ~DegradeAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    // DSP stages
    SoftClipper       softClipper;
    PreEmphShelf      preEmphShelf;
    BitCrusher        bitCrusher;
    SampleRateReducer sampleRateReducer;
    SpectralSmearer   spectralSmearerL;
    SpectralSmearer   spectralSmearerR;
    PostLPF           postLPF;

    // Smoothed parameters (7 continuous params)
    juce::SmoothedValue<float> smoothDrive, smoothClip, smoothPreEmph, smoothPostCutoff, smoothMix;
    juce::SmoothedValue<float> smoothSmear, smoothSpectralCrush;

    // 768-sample dry delay ring buffer for latency-compensated Mix crossfade (DSP-07).
    // Size is latencySamples + 1 = 769: writing at W and reading at (W+1)%769 gives
    // exactly 768 samples of delay (oldest sample is (size-1) positions behind write head).
    static constexpr int dryDelaySize = SpectralSmearer::latencySamples + 1; // 769
    std::array<float, SpectralSmearer::latencySamples + 1> dryDelayL {};
    std::array<float, SpectralSmearer::latencySamples + 1> dryDelayR {};
    int dryDelayWritePos = 0;

    // Cached host sample rate for S&H ratio calculation
    double currentSampleRate = 44100.0;

    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DegradeAudioProcessor)
};
