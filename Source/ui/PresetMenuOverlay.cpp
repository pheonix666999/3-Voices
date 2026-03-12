#include "PresetMenuOverlay.h"

#include <cmath>

PresetMenuOverlay::PresetMenuOverlay()
{
    setOpaque(false);
    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);

    for (int i = 0; i < 6; ++i)
        categoryRowRectsRef[(size_t) i] = { 24, 27 + i * 49, 585, 43 };

    for (int i = 0; i < 10; ++i)
        presetRowRectsRef[(size_t) i] = { 24, 27 + i * 49, 585, 43 };
}

void PresetMenuOverlay::setMenuImages(const juce::Image*,
                                      const juce::Image*,
                                      const juce::Image*,
                                      const juce::Image*,
                                      const juce::Image*,
                                      const juce::Image*,
                                      const juce::Image*)
{
}

void PresetMenuOverlay::setPresetLibrary(const juce::StringArray& flattenedChoices)
{
    categoryNames.clear();
    presetsByCategory.clear();

    for (const auto& choice : flattenedChoices)
    {
        const auto separator = choice.indexOf(" - ");
        if (separator < 0)
            continue;

        const auto category = choice.substring(0, separator).trim();
        const auto preset = choice.substring(separator + 3).trim();
        const int categoryIndex = categoryNames.indexOf(category);

        if (categoryIndex < 0)
        {
            categoryNames.add(category);
            presetsByCategory.push_back({ preset });
        }
        else
        {
            presetsByCategory[(size_t) categoryIndex].add(preset);
        }
    }
}

void PresetMenuOverlay::setCallbacks(PresetSelected onPresetSelectedIn, Closed onClosedIn)
{
    onPresetSelected = std::move(onPresetSelectedIn);
    onClosed = std::move(onClosedIn);
}

void PresetMenuOverlay::openCategories()
{
    currentCategory = -1;
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
    repaint();
}

void PresetMenuOverlay::paint(juce::Graphics& g)
{
    auto menuScaled = scaleRect(getCurrentMenuRectRef()).toFloat();
    const float corner = menuScaled.getHeight() * 0.045f;
    const auto currentTitle = currentCategory < 0 ? juce::String("SELECT CATEGORY")
                                                  : categoryNames[currentCategory].toUpperCase();

    auto drawDisplayText = [&g](juce::String text, juce::Rectangle<int> area, juce::Justification justification)
    {
        auto font = juce::Font(juce::FontOptions(area.getHeight() * 0.58f));
        font.setHorizontalScale(1.12f);
        g.setFont(font);

        g.setColour(juce::Colour(0xFF53E68A).withAlpha(0.14f));
        g.drawFittedText(text, area.translated(0, 1), justification, 1);
        g.setColour(juce::Colour(0xFF7AF5A4).withAlpha(0.12f));
        g.drawFittedText(text, area.expanded(2, 0), justification, 1);
        g.setColour(juce::Colour(0xFF79F79D));
        g.drawFittedText(text, area, justification, 1);
    };

    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(menuScaled, corner);
    g.setColour(juce::Colour(0xFF79F79D).withAlpha(0.18f));
    g.drawRoundedRectangle(menuScaled.reduced(1.5f), corner, 1.0f);

    auto titleArea = menuScaled.removeFromTop(menuScaled.getHeight() * 0.09f);
    drawDisplayText(currentTitle, titleArea.toNearestInt().reduced(14, 2), juce::Justification::centredLeft);

    if (currentCategory >= 0)
    {
        const auto backArea = scaleRect(backRectRef);
        drawDisplayText("< BACK",
                        backArea.translated((int) std::round(menuScaled.getX()),
                                            (int) std::round(menuScaled.getY())).toNearestInt(),
                        juce::Justification::centredLeft);
    }

    const auto rowBase = scaleRect(getCurrentMenuRectRef()).getPosition();

    if (currentCategory < 0)
    {
        for (int i = 0; i < categoryNames.size() && i < (int) categoryRowRectsRef.size(); ++i)
        {
            const auto row = scaleRect(categoryRowRectsRef[(size_t) i]).translated(rowBase.x, rowBase.y).toFloat();
            g.setColour(juce::Colours::white.withAlpha(0.04f));
            g.fillRoundedRectangle(row, 6.0f);
            g.setColour(juce::Colour(0xFF79F79D).withAlpha(0.12f));
            g.drawRoundedRectangle(row.reduced(0.8f), 6.0f, 0.8f);
            drawDisplayText(categoryNames[i].toUpperCase(), row.toNearestInt().reduced(14, 0),
                            juce::Justification::centredLeft);
        }
        return;
    }

    const auto& presets = presetsByCategory[(size_t) currentCategory];
    for (int i = 0; i < presets.size() && i < (int) presetRowRectsRef.size(); ++i)
    {
        const auto row = scaleRect(presetRowRectsRef[(size_t) i]).translated(rowBase.x, rowBase.y).toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.fillRoundedRectangle(row, 6.0f);
        g.setColour(juce::Colour(0xFF79F79D).withAlpha(0.12f));
        g.drawRoundedRectangle(row.reduced(0.8f), 6.0f, 0.8f);
        drawDisplayText(presets[i].toUpperCase(), row.toNearestInt().reduced(14, 0),
                        juce::Justification::centredLeft);
    }
}

void PresetMenuOverlay::resized()
{
    repaint();
}

void PresetMenuOverlay::mouseUp(const juce::MouseEvent& event)
{
    const auto screenMenuRef = getCurrentMenuRectRef();
    const auto screenMenuRect = scaleRect(screenMenuRef);

    if (!screenMenuRect.contains(event.getPosition()))
    {
        if (onClosed)
            onClosed();
        return;
    }

    const auto scaleX = screenMenuRect.getWidth() / (float) screenMenuRef.getWidth();
    const auto scaleY = screenMenuRect.getHeight() / (float) screenMenuRef.getHeight();

    const int localX = (int) std::round((event.x - screenMenuRect.getX()) / juce::jmax(0.001f, scaleX));
    const int localY = (int) std::round((event.y - screenMenuRect.getY()) / juce::jmax(0.001f, scaleY));
    const juce::Point<int> localPt { localX, localY };

    if (currentCategory < 0)
    {
        for (int i = 0; i < categoryNames.size() && i < (int) categoryRowRectsRef.size(); ++i)
        {
            if (categoryRowRectsRef[(size_t) i].contains(localPt))
            {
                currentCategory = i;
                repaint();
                return;
            }
        }
        return;
    }

    if (backRectRef.contains(localPt))
    {
        currentCategory = -1;
        repaint();
        return;
    }

    const auto& presets = presetsByCategory[(size_t) currentCategory];
    for (int i = 0; i < presets.size() && i < (int) presetRowRectsRef.size(); ++i)
    {
        if (presetRowRectsRef[(size_t) i].contains(localPt))
        {
            if (onPresetSelected)
                onPresetSelected(currentCategory, i);

            if (onClosed)
                onClosed();
            return;
        }
    }
}

bool PresetMenuOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (currentCategory < 0)
        {
            if (onClosed)
                onClosed();
        }
        else
        {
            currentCategory = -1;
            repaint();
        }
        return true;
    }

    return false;
}

juce::Rectangle<int> PresetMenuOverlay::scaleRect(const juce::Rectangle<int>& ref) const
{
    const float scaleX = getWidth() / (float) designW;
    const float scaleY = getHeight() / (float) designH;
    const float uniformScale = juce::jmin(scaleX, scaleY);
    const float offsetX = (getWidth() - designW * uniformScale) * 0.5f;
    const float offsetY = (getHeight() - designH * uniformScale) * 0.5f;

    return {
        (int) std::round(ref.getX() * uniformScale + offsetX),
        (int) std::round(ref.getY() * uniformScale + offsetY),
        (int) std::round(ref.getWidth() * uniformScale),
        (int) std::round(ref.getHeight() * uniformScale)
    };
}

juce::Rectangle<int> PresetMenuOverlay::getCurrentMenuRectRef() const
{
    return currentCategory < 0 ? categoriesMenuRef : presetsMenuRef;
}
