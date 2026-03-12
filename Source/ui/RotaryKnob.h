#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class RotaryKnob : public juce::Slider
{
public:
    RotaryKnob()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.15f,
                            juce::MathConstants<float>::pi * 2.85f,
                            true);
    }
};
