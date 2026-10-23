#include "PluginProcessor.h"
#include "PluginEditor.h"

DegradeAudioProcessor::DegradeAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("DegradeParams"), createParameterLayout())
{
}

DegradeAudioProcessor::~DegradeAudioProcessor() {}

const juce::String DegradeAudioProcessor::getName() const { return JucePlugin_Name; }
bool DegradeAudioProcessor::acceptsMidi() const { return false; }
bool DegradeAudioProcessor::producesMidi() const { return false; }
bool DegradeAudioProcessor::isMidiEffect() const { return false; }
double DegradeAudioProcessor::getTailLengthSeconds() const { return 0.0; }

// ---------------------------------------------------------------------------
// Factory presets
// ---------------------------------------------------------------------------

namespace {
    struct Preset {
        const char* name;
        float drive;        // 0-24 dB
        float clip;         // 0-1
        float preEmph;      // 0-1
        float bitDepth;     // 4-24 (integer steps)
        float sampleRate;   // 4000-44100 Hz
        float smear;        // 0-1
        float specCrush;    // 0-1
        float postCutoff;   // 1000-20000 Hz
        float mix;          // 0-1
    };

    static const Preset kPresets[] = {
        // name                  drive  clip   preEmph  bits    sr      smear  crush  cutoff   mix
        { "128kbps Jesus",        2.0f, 0.00f,  0.10f,  20.0f, 44100.f, 0.06f, 0.08f, 18000.f, 1.00f },
        { "Napster Ghost",        4.0f, 0.03f,  0.20f,  16.0f, 32000.f, 0.20f, 0.12f, 16000.f, 0.85f },
        { "Bitrot Elegy",         5.0f, 0.06f,  0.25f,  14.0f, 22050.f, 0.30f, 0.20f, 14000.f, 0.80f },
        { "RealPlayer Tomb",      3.0f, 0.04f,  0.15f,  14.0f, 22050.f, 0.10f, 0.20f,  8000.f, 1.00f },
        { "WinAmp Funeral",       4.0f, 0.05f,  0.20f,  16.0f, 32000.f, 0.15f, 0.15f, 15000.f, 0.75f },
        { "ICQ Transmission",     3.0f, 0.02f,  0.40f,  11.0f,  8000.f, 0.12f, 0.08f,  6000.f, 1.00f },
        { "Dial-Up Seance",       6.0f, 0.08f,  0.30f,  12.0f, 11025.f, 0.35f, 0.25f, 10000.f, 1.00f },
        { "Kazaa Worm",          12.0f, 0.30f,  0.50f,   8.0f,  8000.f, 0.55f, 0.45f,  7000.f, 0.90f },
        { "56k Requiem",          8.0f, 0.15f,  0.50f,   8.0f,  6000.f, 0.50f, 0.40f,  5000.f, 0.90f },
        { "Bin Heaven",          20.0f, 0.60f,  0.80f,   4.0f,  4000.f, 0.85f, 0.80f,  1500.f, 1.00f },
    };

    static constexpr int kNumPresets = (int)(sizeof(kPresets) / sizeof(kPresets[0]));

    void setParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float rawValue)
    {
        auto* p = apvts.getParameter(id);
        if (p != nullptr)
            p->setValueNotifyingHost(p->convertTo0to1(rawValue));
    }
}

int DegradeAudioProcessor::getNumPrograms() { return kNumPresets; }
int DegradeAudioProcessor::getCurrentProgram() { return currentProgram; }

void DegradeAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= kNumPresets)
        return;
    currentProgram = index;
    const auto& p = kPresets[index];
    setParam(apvts, ParamIDs::DRIVE,       p.drive);
    setParam(apvts, ParamIDs::CLIP,        p.clip);
    setParam(apvts, ParamIDs::PRE_EMPH,    p.preEmph);
    setParam(apvts, ParamIDs::BIT_DEPTH,   p.bitDepth);
    setParam(apvts, ParamIDs::SAMPLE_RATE, p.sampleRate);
    setParam(apvts, ParamIDs::SMEAR,       p.smear);
    setParam(apvts, ParamIDs::SPEC_CRUSH,  p.specCrush);
    setParam(apvts, ParamIDs::POST_CUTOFF, p.postCutoff);
    setParam(apvts, ParamIDs::MIX,         p.mix);
}

const juce::String DegradeAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < kNumPresets)
        return kPresets[index].name;
    return {};
}

void DegradeAudioProcessor::changeProgramName(int, const juce::String&) {}

void DegradeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare all DSP stages
    softClipper.prepare(sampleRate, samplesPerBlock);
    preEmphShelf.prepare(sampleRate, samplesPerBlock);
    bitCrusher.prepare(sampleRate, samplesPerBlock);
    sampleRateReducer.prepare(sampleRate, samplesPerBlock);
    spectralSmearerL.prepare(sampleRate, samplesPerBlock);
    spectralSmearerR.prepare(sampleRate, samplesPerBlock);
    postLPF.prepare(sampleRate, samplesPerBlock);

    // Report FFT latency to host (INF-05) -- always on, never changes
    setLatencySamples(SpectralSmearer::latencySamples); // 768

    // Initialize smoothers: reset ramp time, then set current value from APVTS
    // Gain/blend params: 50ms (musical/slow)
    smoothDrive.reset(sampleRate, 0.050);
    smoothDrive.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::DRIVE));

    smoothClip.reset(sampleRate, 0.050);
    smoothClip.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::CLIP));

    smoothMix.reset(sampleRate, 0.050);
    smoothMix.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::MIX));

    // Filter/emphasis params: 10ms (tight/responsive)
    smoothPreEmph.reset(sampleRate, 0.010);
    smoothPreEmph.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::PRE_EMPH));

    smoothPostCutoff.reset(sampleRate, 0.010);
    smoothPostCutoff.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::POST_CUTOFF));

    // Spectral params: tight/responsive (10ms)
    smoothSmear.reset(sampleRate, 0.010);
    smoothSmear.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::SMEAR));

    smoothSpectralCrush.reset(sampleRate, 0.010);
    smoothSpectralCrush.setCurrentAndTargetValue(*apvts.getRawParameterValue(ParamIDs::SPEC_CRUSH));

    // Initialize dry delay buffer
    dryDelayL.fill(0.0f);
    dryDelayR.fill(0.0f);
    dryDelayWritePos = 0;
}

void DegradeAudioProcessor::releaseResources()
{
    softClipper.reset();
    preEmphShelf.reset();
    bitCrusher.reset();
    sampleRateReducer.reset();
    spectralSmearerL.reset();
    spectralSmearerR.reset();
    postLPF.reset();
    dryDelayL.fill(0.0f);
    dryDelayR.fill(0.0f);
    dryDelayWritePos = 0;
}

bool DegradeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void DegradeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read unsmoothed discrete params once per block (std::round avoids float truncation, Pitfall 6)
    const int   bitDepth = (int)std::round(*apvts.getRawParameterValue(ParamIDs::BIT_DEPTH));
    const float targetSR = *apvts.getRawParameterValue(ParamIDs::SAMPLE_RATE);

    // Set targets on smoothed params
    smoothDrive.setTargetValue(*apvts.getRawParameterValue(ParamIDs::DRIVE));
    smoothClip.setTargetValue(*apvts.getRawParameterValue(ParamIDs::CLIP));
    smoothPreEmph.setTargetValue(*apvts.getRawParameterValue(ParamIDs::PRE_EMPH));
    smoothPostCutoff.setTargetValue(*apvts.getRawParameterValue(ParamIDs::POST_CUTOFF));
    smoothMix.setTargetValue(*apvts.getRawParameterValue(ParamIDs::MIX));
    smoothSmear.setTargetValue(*apvts.getRawParameterValue(ParamIDs::SMEAR));
    smoothSpectralCrush.setTargetValue(*apvts.getRawParameterValue(ParamIDs::SPEC_CRUSH));

    const int numSamples = buffer.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        // Advance smoothers once per sample
        const float driveDB       = smoothDrive.getNextValue();
        const float clip          = smoothClip.getNextValue();
        const float preEmph       = smoothPreEmph.getNextValue();
        const float postCutoff    = smoothPostCutoff.getNextValue();
        const float mix           = smoothMix.getNextValue();
        const float smear         = smoothSmear.getNextValue();
        const float spectralCrush = smoothSpectralCrush.getNextValue();

        // Recompute post-LPF coefficient per sample (one exp() -- acceptable cost, Pitfall 5)
        postLPF.updateCoeff(postCutoff);

        for (int ch = 0; ch < 2; ++ch)
        {
            float* data     = buffer.getWritePointer(ch);
            const float dry = data[s];

            // Write dry into delay ring buffer, read 768-sample delayed dry (DSP-07)
            auto& delayBuf = (ch == 0) ? dryDelayL : dryDelayR;
            delayBuf[dryDelayWritePos] = dry;
            int delayReadPos = (dryDelayWritePos + 1) % dryDelaySize;
            float delayedDry = delayBuf[delayReadPos];

            float x = dry;

            // Stage 1: Drive + tanh soft clipper (DSP-01)
            x = softClipper.processSample(x, driveDB, clip);

            // Stage 2: Pre-emphasis high shelf (DSP-02)
            x = preEmphShelf.processSample(x, ch, preEmph);

            // Stage 3: Bit depth reducer (DSP-03)
            x = bitCrusher.processSample(x, bitDepth);

            // Stage 4: Sample rate reducer S&H (DSP-04)
            x = sampleRateReducer.processSample(x, ch, targetSR);

            // Stage 5: SpectralSmearer -- overlap-add FFT (DSP-05)
            auto& smearer = (ch == 0) ? spectralSmearerL : spectralSmearerR;
            smearer.pushSample(x);
            if (smearer.isHopReady())
                smearer.processFrame(smear, spectralCrush);
            float fftOut = smearer.popSample();

            // Stage 6: Post low-pass (DSP-06)
            fftOut = postLPF.processSample(fftOut, ch);

            // Latency-compensated wet/dry crossfade (DSP-07)
            data[s] = delayedDry + mix * (fftOut - delayedDry);
        }

        // Advance dry delay write position once per sample (shared across channels)
        dryDelayWritePos = (dryDelayWritePos + 1) % dryDelaySize;
    }
}

bool DegradeAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* DegradeAudioProcessor::createEditor()
{
    return new DegradeAudioProcessorEditor(*this);
}

void DegradeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DegradeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DegradeAudioProcessor();
}
