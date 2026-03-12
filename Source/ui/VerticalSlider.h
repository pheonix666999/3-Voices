#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VerticalSlider : public juce::Slider
{
public:
    VerticalSlider()
    {
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }
};
