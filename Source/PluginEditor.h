#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom rotary knob that matches the mockup
class CustomKnob : public juce::Slider
{
public:
    CustomKnob()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF9E9E9E));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF9E9E9E));
    }
};

//==============================================================================
// Custom vertical fader
class CustomFader : public juce::Slider
{
public:
    CustomFader()
    {
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }
};

//==============================================================================
// Custom toggle button for voices
class VoiceButton : public juce::ToggleButton
{
public:
    VoiceButton(const juce::String& text) : label(text)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        auto bounds = getLocalBounds().toFloat();

        // Orange background
        g.setColour(juce::Colour(0xFFFFA726));
        g.fillRect(bounds);

        // Border when toggled
        if (getToggleState())
        {
            g.setColour(juce::Colours::white);
            g.drawRect(bounds, 3.0f);
        }

        // Label text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText(label, bounds, juce::Justification::centred);
    }

private:
    juce::String label;
};

//==============================================================================
// Custom toggle button for Tube/Bit
class DistortionTypeButton : public juce::ToggleButton
{
public:
    DistortionTypeButton(const juce::String& text) : label(text)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        auto bounds = getLocalBounds().toFloat();

        // Orange background
        g.setColour(juce::Colour(0xFFFFA726));
        g.fillRect(bounds);

        // Darker when not active
        if (!getToggleState())
        {
            g.setColour(juce::Colour(0x80000000));
            g.fillRect(bounds);
        }

        // Border when toggled
        if (getToggleState())
        {
            g.setColour(juce::Colours::white);
            g.drawRect(bounds, 2.0f);
        }

        // Label text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f));
        g.drawText(label, bounds, juce::Justification::centred);
    }

private:
    juce::String label;
};

//==============================================================================
// Custom look and feel for the plugin
class ThreeVoicesLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThreeVoicesLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.3f));
        setColour(juce::Slider::backgroundColourId, juce::Colours::white.withAlpha(0.1f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider& slider) override
    {
        juce::ignoreUnused(slider);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;

        // Draw knob body (gray circle)
        g.setColour(juce::Colour(0xFF9E9E9E));
        g.fillEllipse(rx, ry, rw, rw);

        // Draw indicator dot
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto dotRadius = radius * 0.15f;
        auto dotDistance = radius * 0.6f;
        auto dotX = centreX + dotDistance * std::cos(angle - juce::MathConstants<float>::halfPi);
        auto dotY = centreY + dotDistance * std::sin(angle - juce::MathConstants<float>::halfPi);

        g.setColour(juce::Colours::black);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        juce::ignoreUnused(minSliderPos, maxSliderPos, style, slider);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

        g.setColour(juce::Colours::white);

        // Draw track (vertical white line)
        auto trackWidth = 4.0f;
        auto trackX = bounds.getCentreX() - trackWidth / 2.0f;
        g.fillRect(trackX, bounds.getY(), trackWidth, bounds.getHeight());

        // Draw top cap (horizontal line) - matches mockup
        auto capWidth = bounds.getWidth() * 0.9f;
        auto capHeight = 4.0f;
        auto capX = bounds.getCentreX() - capWidth / 2.0f;
        g.fillRect(capX, bounds.getY(), capWidth, capHeight);

        // Draw bottom cap (horizontal line)
        g.fillRect(capX, bounds.getBottom() - capHeight, capWidth, capHeight);

        // Draw thumb (T-shaped handle)
        auto thumbHeight = 10.0f;
        auto thumbWidth = bounds.getWidth();
        auto thumbY = sliderPos - thumbHeight / 2.0f;

        // Clamp thumb position within bounds
        thumbY = juce::jlimit(bounds.getY(), bounds.getBottom() - thumbHeight, thumbY);

        g.fillRect(bounds.getX(), thumbY, thumbWidth, thumbHeight);
    }
};

//==============================================================================
class ThreeVoicesAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit ThreeVoicesAudioProcessorEditor(ThreeVoicesAudioProcessor&);
    ~ThreeVoicesAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ThreeVoicesAudioProcessor& audioProcessor;
    ThreeVoicesLookAndFeel customLookAndFeel;

    // Input/Output Gain
    CustomFader inputGainSlider;
    CustomFader outputGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    // Global controls
    CustomKnob mixKnob;
    CustomFader widthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;

    // Voice controls (3 voices)
    struct VoiceControls
    {
        std::unique_ptr<VoiceButton> voiceButton;
        std::unique_ptr<CustomKnob> speedKnob;
        std::unique_ptr<CustomKnob> delayTimeKnob;
        std::unique_ptr<CustomKnob> depthKnob;
        std::unique_ptr<CustomFader> distortionSlider;
        std::unique_ptr<DistortionTypeButton> tubeButton;
        std::unique_ptr<DistortionTypeButton> bitButton;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> voiceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distortionAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tubeAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bitAttachment;
    };

    VoiceControls voiceControls[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeVoicesAudioProcessorEditor)
};
