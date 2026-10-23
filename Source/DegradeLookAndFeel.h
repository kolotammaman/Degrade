#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class DegradeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr juce::uint32 bgColourHex    = 0xFF080C08;
    static constexpr juce::uint32 phosphorHex    = 0xFF5A8A5A;
    static constexpr juce::uint32 activeGlowHex  = 0xFF7AAD7A;
    static constexpr juce::uint32 dimTextHex     = 0xFF3D6B3D;
    static constexpr juce::uint32 trackFillHex   = 0xFF1A2E1A;

    static const juce::Colour bgColour;
    static const juce::Colour phosphor;
    static const juce::Colour activeGlow;
    static const juce::Colour dimText;
    static const juce::Colour trackFill;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const bool grabbed = slider.isMouseButtonDown();
        const juce::Colour thumbColour  = grabbed ? activeGlow : phosphor;
        const juce::Colour fillColour   = grabbed ? activeGlow : trackFill;

        const int trackX = x + width / 2 - 1;
        const int trackTop = y;
        const int trackBottom = y + height;

        // Draw track: thin vertical line
        g.setColour(dimText);
        g.fillRect(trackX, trackTop, 2, trackBottom - trackTop);

        // Draw below-thumb fill
        const int thumbY = static_cast<int>(sliderPos);
        if (thumbY < trackBottom)
        {
            g.setColour(fillColour);
            g.fillRect(trackX, thumbY, 2, trackBottom - thumbY);
        }

        // Draw thumb: flat horizontal rect
        const int thumbW = static_cast<int>(width * 0.6f);
        const int thumbH = 4;
        const int thumbX = x + (width - thumbW) / 2;
        g.setColour(thumbColour);
        g.fillRect(thumbX, thumbY - thumbH / 2, thumbW, thumbH);
    }

    juce::Label* createSliderTextBox(juce::Slider&) override
    {
        auto* label = new juce::Label();
        label->setColour(juce::Label::textColourId,       phosphor);
        label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setColour(juce::Label::outlineColourId,    juce::Colours::transparentBlack);
        label->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        label->setJustificationType(juce::Justification::centred);
        return label;
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll(juce::Colours::transparentBlack);
        g.setColour(dimText);
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        g.drawText(label.getText().toUpperCase(),
                   label.getLocalBounds(),
                   juce::Justification::centred,
                   false);
    }
};

inline const juce::Colour DegradeLookAndFeel::bgColour   { bgColourHex   };
inline const juce::Colour DegradeLookAndFeel::phosphor   { phosphorHex   };
inline const juce::Colour DegradeLookAndFeel::activeGlow { activeGlowHex };
inline const juce::Colour DegradeLookAndFeel::dimText    { dimTextHex    };
inline const juce::Colour DegradeLookAndFeel::trackFill  { trackFillHex  };
