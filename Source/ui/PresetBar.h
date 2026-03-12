#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PresetBar : public juce::Component
{
public:
    PresetBar()
    {
        addAndMakeVisible(previousButton);
        addAndMakeVisible(selectButton);
        addAndMakeVisible(nextButton);

        // Style the buttons with dark backgrounds
        for (auto* btn : { &previousButton, &selectButton, &nextButton })
        {
            btn->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            btn->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF6ACE8C));
            btn->setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF8BE0A6));
        }

        previousButton.setButtonText("<");
        nextButton.setButtonText(">");

        previousButton.onClick = [this] { if (onPrevious != nullptr) onPrevious(); };
        nextButton.onClick = [this] { if (onNext != nullptr) onNext(); };
        selectButton.onClick = [this] { if (onOpenMenu != nullptr) onOpenMenu(); };
    }

    void setPresetName(const juce::String& name, bool dirty)
    {
        selectButton.setButtonText(dirty ? name + " *" : name);
    }

    void paint(juce::Graphics& g) override
    {
        // Draw the PRESET SELECT text with green glow
        auto b = getLocalBounds().toFloat();
        auto textArea = b.reduced(b.getWidth() * 0.1f, 0.0f);

        // Green glow behind text
        g.setColour(juce::Colour(0xFF43A464).withAlpha(0.08f));
        g.fillRoundedRectangle(b, 8.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto navWidth = juce::jmax(32, r.getHeight());

        auto left = r.removeFromLeft(navWidth);
        auto right = r.removeFromRight(navWidth);

        previousButton.setBounds(left.reduced(2));
        nextButton.setBounds(right.reduced(2));
        selectButton.setBounds(r.reduced(2));
    }

    std::function<void()> onOpenMenu;
    std::function<void()> onPrevious;
    std::function<void()> onNext;

private:
    juce::TextButton previousButton;
    juce::TextButton selectButton;
    juce::TextButton nextButton;
};
