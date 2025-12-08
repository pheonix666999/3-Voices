#include "PluginProcessor.h"
#include "PluginEditor.h"

ThreeVoicesAudioProcessorEditor::ThreeVoicesAudioProcessorEditor(ThreeVoicesAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);

    // Set up input gain slider
    inputGainSlider.setRange(-24.0, 24.0, 0.1);
    addAndMakeVisible(inputGainSlider);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputGain", inputGainSlider);

    // Set up output gain slider
    outputGainSlider.setRange(-24.0, 24.0, 0.1);
    addAndMakeVisible(outputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "outputGain", outputGainSlider);

    // Set up mix knob
    mixKnob.setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(mixKnob);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixKnob);

    // Set up width slider
    widthSlider.setRange(0.0, 100.0, 0.1);
    addAndMakeVisible(widthSlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "width", widthSlider);

    // Set up voice controls
    juce::String romanNumerals[] = { "I", "II", "III" };

    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "voice" + juce::String(i + 1);

        // Voice button
        voiceControls[i].voiceButton = std::make_unique<VoiceButton>(romanNumerals[i]);
        addAndMakeVisible(*voiceControls[i].voiceButton);
        voiceControls[i].voiceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), prefix + "On", *voiceControls[i].voiceButton);

        // Speed knob
        voiceControls[i].speedKnob = std::make_unique<CustomKnob>();
        voiceControls[i].speedKnob->setRange(0.1, 10.0, 0.01);
        addAndMakeVisible(*voiceControls[i].speedKnob);
        voiceControls[i].speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "Speed", *voiceControls[i].speedKnob);

        // Delay Time knob
        voiceControls[i].delayTimeKnob = std::make_unique<CustomKnob>();
        voiceControls[i].delayTimeKnob->setRange(1.0, 50.0, 0.1);
        addAndMakeVisible(*voiceControls[i].delayTimeKnob);
        voiceControls[i].delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "DelayTime", *voiceControls[i].delayTimeKnob);

        // Depth knob
        voiceControls[i].depthKnob = std::make_unique<CustomKnob>();
        voiceControls[i].depthKnob->setRange(0.0, 100.0, 0.1);
        addAndMakeVisible(*voiceControls[i].depthKnob);
        voiceControls[i].depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "Depth", *voiceControls[i].depthKnob);

        // Distortion slider
        voiceControls[i].distortionSlider = std::make_unique<CustomFader>();
        voiceControls[i].distortionSlider->setRange(0.0, 100.0, 0.1);
        addAndMakeVisible(*voiceControls[i].distortionSlider);
        voiceControls[i].distortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), prefix + "Distortion", *voiceControls[i].distortionSlider);

        // Tube button
        voiceControls[i].tubeButton = std::make_unique<DistortionTypeButton>("Tube");
        addAndMakeVisible(*voiceControls[i].tubeButton);
        voiceControls[i].tubeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), prefix + "Tube", *voiceControls[i].tubeButton);

        // Bit button
        voiceControls[i].bitButton = std::make_unique<DistortionTypeButton>("Bit");
        addAndMakeVisible(*voiceControls[i].bitButton);
        voiceControls[i].bitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), prefix + "Bit", *voiceControls[i].bitButton);
    }

    // Match the mockup dimensions (approximately)
    setSize(950, 550);
}

ThreeVoicesAudioProcessorEditor::~ThreeVoicesAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ThreeVoicesAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Blue background matching the mockup exactly
    g.fillAll(juce::Colour(0xFF4A7FD6));

    g.setColour(juce::Colours::white);

    // Draw column labels at the top
    g.setFont(juce::Font(16.0f));
    g.drawText("Voices", 100, 55, 80, 25, juce::Justification::centred);
    g.drawText("Speed", 220, 55, 80, 25, juce::Justification::centred);
    g.drawText("Delay Time", 320, 55, 100, 25, juce::Justification::centred);
    g.drawText("Depth", 450, 55, 80, 25, juce::Justification::centred);

    // Draw title on the right side
    g.setFont(juce::Font(28.0f, juce::Font::bold));
    g.drawText("3 Voice", 720, 170, 180, 40, juce::Justification::centred);
    g.drawText("Unison Mod", 720, 210, 180, 40, juce::Justification::centred);

    // Draw Width and Mix labels at the bottom
    g.setFont(juce::Font(16.0f));
    g.drawText("Width", 630, 470, 60, 25, juce::Justification::centred);
    g.drawText("Mix", 820, 470, 60, 25, juce::Justification::centred);
}

void ThreeVoicesAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Layout constants matching the mockup
    const int leftMargin = 40;
    const int rightMargin = 40;
    const int faderWidth = 16;
    const int faderHeight = 450;
    const int knobSize = 75;
    const int voiceButtonWidth = 75;
    const int voiceButtonHeight = 100;
    const int voiceRowHeight = 135;
    const int voiceStartY = 90;
    const int distFaderHeight = 100;
    const int distButtonWidth = 55;
    const int distButtonHeight = 28;

    // Input gain fader (left side with cap lines like in mockup)
    inputGainSlider.setBounds(leftMargin, 50, faderWidth, faderHeight);

    // Output gain fader (right side)
    outputGainSlider.setBounds(bounds.getWidth() - rightMargin - faderWidth, 50, faderWidth, faderHeight);

    // Voice rows positioning
    int voiceButtonX = 95;
    int knobStartX = 185;
    int knobSpacing = 110;
    int distFaderX = 530;
    int distButtonX = 570;

    for (int i = 0; i < 3; ++i)
    {
        int rowY = voiceStartY + i * voiceRowHeight;

        // Voice button (orange box with roman numeral)
        voiceControls[i].voiceButton->setBounds(voiceButtonX, rowY, voiceButtonWidth, voiceButtonHeight);

        // Three knobs in a row
        voiceControls[i].speedKnob->setBounds(knobStartX, rowY + 15, knobSize, knobSize);
        voiceControls[i].delayTimeKnob->setBounds(knobStartX + knobSpacing, rowY + 15, knobSize, knobSize);
        voiceControls[i].depthKnob->setBounds(knobStartX + knobSpacing * 2, rowY + 15, knobSize, knobSize);

        // Distortion fader (vertical fader with T-shaped handle)
        voiceControls[i].distortionSlider->setBounds(distFaderX, rowY + 5, faderWidth, distFaderHeight);

        // Tube and Bit buttons stacked vertically
        voiceControls[i].tubeButton->setBounds(distButtonX, rowY + 10, distButtonWidth, distButtonHeight);
        voiceControls[i].bitButton->setBounds(distButtonX, rowY + 45, distButtonWidth, distButtonHeight);
    }

    // Width slider at the bottom
    widthSlider.setBounds(640, 370, faderWidth, 100);

    // Mix knob at the bottom right (larger knob)
    mixKnob.setBounds(785, 360, 90, 90);
}
