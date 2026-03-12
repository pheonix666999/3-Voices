#pragma once

#include <array>
#include <functional>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>

class PresetMenuOverlay : public juce::Component
{
public:
    using PresetSelected = std::function<void(int categoryIndex, int presetIndex)>;
    using Closed = std::function<void()>;

    PresetMenuOverlay();

    void setMenuImages(const juce::Image* categories,
                       const juce::Image* classic,
                       const juce::Image* guitar,
                       const juce::Image* keysSynth,
                       const juce::Image* bass,
                       const juce::Image* drums,
                       const juce::Image* vocals);
    void setPresetLibrary(const juce::StringArray& flattenedChoices);

    void setCallbacks(PresetSelected onPresetSelectedIn, Closed onClosedIn);
    void openCategories();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    juce::Rectangle<int> scaleRect(const juce::Rectangle<int>& ref) const;
    juce::Rectangle<int> getCurrentMenuRectRef() const;

    // TODO: adjust these reference bounds to exactly match your design placement.
    static constexpr int designW = 3366;
    static constexpr int designH = 1945;
    const juce::Rectangle<int> categoriesMenuRef { 2365, 580, 637, 348 };
    const juce::Rectangle<int> presetsMenuRef    { 2365, 580, 637, 543 };

    // TODO: adjust row hitboxes as needed against your final PNGs.
    std::array<juce::Rectangle<int>, 6> categoryRowRectsRef {};
    std::array<juce::Rectangle<int>, 10> presetRowRectsRef {};
    juce::Rectangle<int> backRectRef { 24, 0, 190, 27 };

    juce::StringArray categoryNames;
    std::vector<juce::StringArray> presetsByCategory;
    int currentCategory = -1;

    PresetSelected onPresetSelected;
    Closed onClosed;
};
