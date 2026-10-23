#include "PluginEditor.h"
#include "Parameters.h"

DegradeAudioProcessorEditor::DegradeAudioProcessorEditor(DegradeAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&degradeLookAndFeel);

    // Setup and add all 9 sliders
    auto setupSlider = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);
    };

    setupSlider(driveSlider);
    setupSlider(clipSlider);
    setupSlider(preEmphSlider);
    setupSlider(bitDepthSlider);
    setupSlider(sampleRateSlider);
    setupSlider(smearSlider);
    setupSlider(specCrushSlider);
    setupSlider(postCutoffSlider);
    setupSlider(mixSlider);

    // Setup and add all 9 labels (manually positioned in resized(), no attachToComponent)
    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupLabel(driveLabel,      "DRIVE");
    setupLabel(clipLabel,       "CLIP");
    setupLabel(preEmphLabel,    "PRE-EMPHASIS");
    setupLabel(bitDepthLabel,   "BIT DEPTH");
    setupLabel(sampleRateLabel, "SAMPLE RATE");
    setupLabel(smearLabel,      "SMEAR");
    setupLabel(specCrushLabel,  "SPECTRAL CRUSH");
    setupLabel(postCutoffLabel, "POST CUTOFF");
    setupLabel(mixLabel,        "MIX");

    // Create APVTS attachments (must be after addAndMakeVisible)
    driveAttach      = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::DRIVE,       driveSlider);
    clipAttach       = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::CLIP,        clipSlider);
    preEmphAttach    = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::PRE_EMPH,    preEmphSlider);
    bitDepthAttach   = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::BIT_DEPTH,   bitDepthSlider);
    sampleRateAttach = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::SAMPLE_RATE, sampleRateSlider);
    smearAttach      = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::SMEAR,       smearSlider);
    specCrushAttach  = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::SPEC_CRUSH,  specCrushSlider);
    postCutoffAttach = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::POST_CUTOFF, postCutoffSlider);
    mixAttach        = std::make_unique<SliderAttachment>(processorRef.apvts, ParamIDs::MIX,         mixSlider);

    setSize(600, 500);
}

DegradeAudioProcessorEditor::~DegradeAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void DegradeAudioProcessorEditor::paint(juce::Graphics& g)
{
    // CRT background
    g.fillAll(DegradeLookAndFeel::bgColour);

    // "DEGRADE" title
    g.setColour(DegradeLookAndFeel::phosphor);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 28.0f, juce::Font::plain));
    g.drawText("DEGRADE", juce::Rectangle<int>(0, 10, getWidth(), 36),
               juce::Justification::centred, false);

    // CRT scanlines: 1px lines every 3 rows at ~12% opacity
    g.setColour(juce::Colour(0x1E000000));
    for (int lineY = 0; lineY < getHeight(); lineY += 3)
        g.fillRect(0, lineY, getWidth(), 1);
}

void DegradeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Reserve title header
    area.removeFromTop(50);
    area.reduce(10, 5);

    const int cols   = 3;
    const int rows   = 3;
    const int cellW  = area.getWidth()  / cols;
    const int cellH  = area.getHeight() / rows;
    const int labelH = 18;

    juce::Slider* sliders[9] = {
        &driveSlider,    &clipSlider,       &preEmphSlider,
        &bitDepthSlider, &sampleRateSlider, &smearSlider,
        &specCrushSlider, &postCutoffSlider, &mixSlider
    };

    juce::Label* labels[9] = {
        &driveLabel,    &clipLabel,       &preEmphLabel,
        &bitDepthLabel, &sampleRateLabel, &smearLabel,
        &specCrushLabel, &postCutoffLabel, &mixLabel
    };

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const int idx = row * cols + col;
            const int x   = area.getX() + col * cellW;
            const int y   = area.getY() + row * cellH;

            labels[idx]->setBounds(x, y, cellW, labelH);
            sliders[idx]->setBounds(x, y + labelH, cellW, cellH - labelH);
        }
    }
}
