#pragma once
#include <cmath>
#include <random>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// DSP.h — Five sample-by-sample DSP stage structs for the Degrade plugin.
// Each struct implements prepare(sampleRate, blockSize), reset(), and processSample().
// All per-channel filter state is stored in [2] arrays (L=0, R=1) for stereo independence (INF-02).
// No third-party DSP libraries — only <cmath> and JUCE headers (INF-06).
//==============================================================================

//==============================================================================
// Stage 1: SoftClipper (DSP-01)
// Input gain (Drive dB) feeds a tanh soft clipper.
// y = tanh(k * gain * x) / tanh(k)
// k maps Clip (0-1) to range [1, 10]: k = 1 + clip * 9
// Stateless — no per-channel state.
//==============================================================================
struct SoftClipper
{
    void prepare(double /*sampleRate*/, int /*blockSize*/) {}
    void reset() {}

    float processSample(float x, float driveDB, float clip) const
    {
        float gain = juce::Decibels::decibelsToGain(driveDB);
        float k    = 1.0f + clip * 9.0f;
        float kx   = k * gain * x;
        return std::tanh(kx) / std::tanh(k);
    }
};

//==============================================================================
// Stage 2: PreEmphShelf (DSP-02)
// 1-pole high shelf at ~4 kHz, 0-12 dB boost controlled by PreEmphasis (0-1).
// Additive formulation: y = x + gainLinear * (x - lpf(x))
// The LPF coefficient a1 is computed once at prepare() time from the fixed 4 kHz
// shelf frequency and host sample rate.
// Per-channel state: lpfState[2].
//==============================================================================
struct PreEmphShelf
{
    float a1 = 0.0f;
    float lpfState[2] = {};

    void prepare(double sampleRate, int /*blockSize*/)
    {
        a1 = std::exp(-juce::MathConstants<float>::twoPi * 4000.0f / (float)sampleRate);
        reset();
    }

    void reset()
    {
        lpfState[0] = lpfState[1] = 0.0f;
    }

    float processSample(float x, int ch, float preEmphasis)
    {
        // 1-pole LPF
        float lpfOut  = (1.0f - a1) * x + a1 * lpfState[ch];
        lpfState[ch]  = lpfOut;

        // High shelf: blend original with high-frequency content
        float gainDB     = preEmphasis * 12.0f;
        float gainLinear = juce::Decibels::decibelsToGain(gainDB) - 1.0f;
        return x + gainLinear * (x - lpfOut);
    }
};

//==============================================================================
// Stage 3: BitCrusher (DSP-03)
// Quantizes samples to bitDepth bits.
// Formula: floor(x * steps) / steps,  steps = 2^(bitDepth - 1)
// Stateless — no per-channel state.
//==============================================================================
struct BitCrusher
{
    void prepare(double /*sampleRate*/, int /*blockSize*/) {}
    void reset() {}

    float processSample(float x, int bitDepth) const
    {
        float steps = std::pow(2.0f, (float)(bitDepth - 1));
        return std::floor(x * steps) / steps;
    }
};

//==============================================================================
// Stage 4: SampleRateReducer (DSP-04)
// Sample-and-hold with a per-channel phase accumulator.
// increment = targetSR / hostSR; latch when phase >= 1.0; rollover via floor.
// Initialized with phase = 1.0f to force an immediate latch on the first sample
// so the held value is never zero/stale at plugin load.
// Per-channel state: phase[2], held[2].
//==============================================================================
struct SampleRateReducer
{
    float phase[2]  = {1.0f, 1.0f};  // init to 1.0f to force immediate latch
    float held[2]   = {};
    float hostSR    = 44100.0f;

    void prepare(double sampleRate, int /*blockSize*/)
    {
        hostSR = (float)sampleRate;
        reset();
    }

    void reset()
    {
        phase[0] = phase[1] = 1.0f;
        held[0]  = held[1]  = 0.0f;
    }

    float processSample(float x, int ch, float targetSR)
    {
        phase[ch] += targetSR / hostSR;
        if (phase[ch] >= 1.0f)
        {
            held[ch]   = x;
            phase[ch] -= std::floor(phase[ch]);
        }
        return held[ch];
    }
};

//==============================================================================
// Stage 5: PostLPF (DSP-06)
// Standard 1-pole RC low-pass filter.
// a1 = exp(-2π * fc / sr);  y[n] = (1 - a1) * x[n] + a1 * y[n-1]
// Coefficient is updated per sample (via updateCoeff) since PostCutoff is smoothed
// and can change at audio rate.
// Per-channel state: state[2].
//==============================================================================
struct PostLPF
{
    float a1        = 0.0f;
    float state[2]  = {};
    float currentSR = 44100.0f;

    void prepare(double sampleRate, int /*blockSize*/)
    {
        currentSR = (float)sampleRate;
        reset();
    }

    void reset()
    {
        state[0] = state[1] = 0.0f;
    }

    void updateCoeff(float fc)
    {
        a1 = std::exp(-juce::MathConstants<float>::twoPi * fc / currentSR);
    }

    float processSample(float x, int ch)
    {
        float y    = (1.0f - a1) * x + a1 * state[ch];
        state[ch]  = y;
        return y;
    }
};

//==============================================================================
// Stage 6: SpectralSmearer (DSP-05)
// Overlap-add FFT processor: 1024-point FFT, 75% overlap (256-sample hop), Hann window.
// Latency = fftSize - hopSize = 768 samples.
// Smear: randomizes bin phases (linear interpolation between original and random).
// SpectralCrush: zeros bins with quadratic frequency-weighted probability (high freqs crushed first).
// One SpectralSmearer per channel (L and R are separate instances, INF-02).
//==============================================================================
struct SpectralSmearer
{
    static constexpr int fftOrder        = 10;
    static constexpr int fftSize         = 1 << fftOrder;   // 1024
    static constexpr int hopSize         = fftSize / 4;     // 256 -- 75% overlap
    static constexpr int latencySamples  = fftSize - hopSize; // 768

    juce::dsp::FFT fft { fftOrder };

    std::array<float, fftSize>      window       {};
    std::array<float, fftSize>      inputBuffer  {};
    std::array<float, fftSize * 2>  fftData      {};  // real+imag interleaved for JUCE FFT
    std::array<float, fftSize>      outputBuffer {};

    int inputWritePos        = 0;
    int outputReadPos        = 0;
    int samplesUntilNextHop  = 0;

    std::mt19937 rng { 42 };
    std::uniform_real_distribution<float> phaseDist { -juce::MathConstants<float>::pi,
                                                       juce::MathConstants<float>::pi };
    std::uniform_real_distribution<float> crushDist { 0.0f, 1.0f };

    void prepare(double /*sampleRate*/, int /*blockSize*/)
    {
        // Precompute Hann window
        for (int i = 0; i < fftSize; ++i)
            window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (float)fftSize));

        reset();
    }

    void reset()
    {
        inputBuffer.fill(0.0f);
        fftData.fill(0.0f);
        outputBuffer.fill(0.0f);
        inputWritePos       = 0;
        outputReadPos       = 0;
        samplesUntilNextHop = hopSize;
        rng.seed(42);
    }

    void pushSample(float sample)
    {
        inputBuffer[inputWritePos] = sample;
        inputWritePos = (inputWritePos + 1) % fftSize;
        --samplesUntilNextHop;
    }

    float popSample()
    {
        float out = outputBuffer[outputReadPos];
        outputBuffer[outputReadPos] = 0.0f;
        outputReadPos = (outputReadPos + 1) % fftSize;
        return out;
    }

    bool isHopReady() const
    {
        return samplesUntilNextHop <= 0;
    }

    void processFrame(float smear, float spectralCrush)
    {
        // 1. Window + copy to fftData (read backwards from inputWritePos, which is the most recent write position)
        //    The most recent fftSize samples end at inputWritePos - 1.
        for (int i = 0; i < fftSize; ++i)
        {
            int readIdx = (inputWritePos - fftSize + i + fftSize) % fftSize;
            fftData[i] = inputBuffer[readIdx] * window[i];
        }
        // Zero imaginary part
        for (int i = fftSize; i < fftSize * 2; ++i)
            fftData[i] = 0.0f;

        // 2. Forward FFT
        fft.performRealOnlyForwardTransform(fftData.data());

        // 3. Spectral processing (bins 1 to fftSize/2 - 1, skip DC and Nyquist)
        for (int b = 1; b < fftSize / 2; ++b)
        {
            float re  = fftData[b * 2];
            float im  = fftData[b * 2 + 1];
            float mag = std::sqrt(re * re + im * im);

            // Smear: linear interpolation between original and random phase.
            // smear^2 pre-warp makes the lower half of the knob subtler — codec
            // degradation should feel gentle at low settings, washy at high ones.
            float phase        = std::atan2(im, re);
            float randPhase    = phaseDist(rng);
            float effectSmear  = smear * smear;
            float newPhase     = phase + effectSmear * (randPhase - phase);

            // SpectralCrush: linear frequency-weighted zeroing (high freqs crushed hardest).
            // Linear normFreq (not quadratic) ensures audible content in the 5–15 kHz
            // range is meaningfully affected: at SC=1.0 bin 256 (~11 kHz) has 50%
            // probability of zeroing, bin 128 (~5.5 kHz) has 25%.
            float normFreq = (float)b / (float)(fftSize / 2);
            float prob     = spectralCrush * normFreq;
            if (crushDist(rng) < prob)
                mag = 0.0f;

            // Reconstruct complex bin
            fftData[b * 2]     = mag * std::cos(newPhase);
            fftData[b * 2 + 1] = mag * std::sin(newPhase);
        }

        // 4. Inverse FFT (JUCE does not divide by N)
        fft.performRealOnlyInverseTransform(fftData.data());

        // 5. Overlap-add with synthesis window and normalization.
        //    JUCE's performRealOnlyInverseTransform already divides by fftSize internally,
        //    so only the Hann COLA factor (1.5 for 75% overlap) needs to be removed here.
        const float scale = 1.0f / 1.5f;
        for (int i = 0; i < fftSize; ++i)
        {
            int outIdx = (outputReadPos + i) % fftSize;
            outputBuffer[outIdx] += fftData[i] * window[i] * scale;
        }

        // 6. Reset hop counter
        samplesUntilNextHop += hopSize;
    }
};
