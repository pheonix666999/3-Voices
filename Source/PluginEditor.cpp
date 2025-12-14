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
        voiceControls[i].delayTimeKnob->setRange(0.0, 150.0, 0.1);
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

    // Compact dimensions
    setSize(750, 420);

    // Start timer to update width slider state based on active voices
    startTimer(50); // Check every 50ms
}

ThreeVoicesAudioProcessorEditor::~ThreeVoicesAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ThreeVoicesAudioProcessorEditor::timerCallback()
{
    // Count active voices
    int activeVoiceCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "voice" + juce::String(i + 1);
        bool voiceOn = audioProcessor.getAPVTS().getRawParameterValue(prefix + "On")->load() > 0.5f;
        if (voiceOn) activeVoiceCount++;
    }

    // Enable/disable width slider based on active voice count
    bool shouldEnable = activeVoiceCount >= 2;
    widthSlider.setEnabled(shouldEnable);
    
    // Visually indicate disabled state
    widthSlider.setAlpha(shouldEnable ? 1.0f : 0.5f);
}

void ThreeVoicesAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Blue background matching the mockup exactly
    g.fillAll(juce::Colour(0xFF4A7FD6));

    g.setColour(juce::Colours::white);

    // Draw column labels at the top
    g.setFont(juce::Font(14.0f));
    g.drawText("Voices", 75, 40, 60, 20, juce::Justification::centred);
    g.drawText("Speed", 170, 40, 60, 20, juce::Justification::centred);
    g.drawText("Delay Time", 250, 40, 80, 20, juce::Justification::centred);
    g.drawText("Depth", 350, 40, 60, 20, juce::Justification::centred);

    // Draw title on the right side
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("3 Voice", 570, 130, 140, 30, juce::Justification::centred);
    g.drawText("Unison Mod", 570, 160, 140, 30, juce::Justification::centred);

    // Draw Width and Mix labels at the bottom
    g.setFont(juce::Font(14.0f));
    g.drawText("Width", 500, 360, 50, 20, juce::Justification::centred);
    g.drawText("Mix", 640, 360, 50, 20, juce::Justification::centred);
}

void ThreeVoicesAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Compact layout constants
    const int leftMargin = 30;
    const int rightMargin = 30;
    const int faderWidth = 14;
    const int faderHeight = 340;
    const int knobSize = 60;
    const int voiceButtonWidth = 60;
    const int voiceButtonHeight = 75;
    const int voiceRowHeight = 100;
    const int voiceStartY = 70;
    const int distFaderHeight = 75;
    const int distButtonWidth = 45;
    const int distButtonHeight = 22;

    // Input gain fader (left side)
    inputGainSlider.setBounds(leftMargin, 40, faderWidth, faderHeight);

    // Output gain fader (right side)
    outputGainSlider.setBounds(bounds.getWidth() - rightMargin - faderWidth, 40, faderWidth, faderHeight);

    // Voice rows positioning
    int voiceButtonX = 70;
    int knobStartX = 145;
    int knobSpacing = 85;
    int distFaderX = 420;
    int distButtonX = 455;

    for (int i = 0; i < 3; ++i)
    {
        int rowY = voiceStartY + i * voiceRowHeight;

        // Voice button (orange box with roman numeral)
        voiceControls[i].voiceButton->setBounds(voiceButtonX, rowY, voiceButtonWidth, voiceButtonHeight);

        // Three knobs in a row
        voiceControls[i].speedKnob->setBounds(knobStartX, rowY + 8, knobSize, knobSize);
        voiceControls[i].delayTimeKnob->setBounds(knobStartX + knobSpacing, rowY + 8, knobSize, knobSize);
        voiceControls[i].depthKnob->setBounds(knobStartX + knobSpacing * 2, rowY + 8, knobSize, knobSize);

        // Distortion fader (vertical fader with T-shaped handle)
        voiceControls[i].distortionSlider->setBounds(distFaderX, rowY + 5, faderWidth, distFaderHeight);

        // Tube and Bit buttons stacked vertically
        voiceControls[i].tubeButton->setBounds(distButtonX, rowY + 8, distButtonWidth, distButtonHeight);
        voiceControls[i].bitButton->setBounds(distButtonX, rowY + 35, distButtonWidth, distButtonHeight);
    }

    // Width slider at the bottom
    widthSlider.setBounds(510, 280, faderWidth, 75);

    // Mix knob at the bottom right
    mixKnob.setBounds(620, 270, 70, 70);
}
