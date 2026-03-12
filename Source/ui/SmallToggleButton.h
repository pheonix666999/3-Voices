#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SmallToggleButton : public juce::ToggleButton
{
public:
    explicit SmallToggleButton(const juce::String& text) : label(text)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        auto on = getToggleState();
        const float cornerRadius = juce::jmax(5.0f, juce::jmin(b.getWidth(), b.getHeight()) * 0.18f);

        // === Shadow layers beneath button ===
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(b.translated(2.0f, 3.0f), cornerRadius);

        if (on)
        {
            // === ACTIVE STATE: bold green ===

            // Outer glow
            g.setColour(juce::Colour(0xFF43A464).withAlpha(0.28f));
            g.fillRoundedRectangle(b.expanded(3.0f), cornerRadius + 3.0f);

            // Dark bevel border
            g.setColour(juce::Colour(0xFF1E5030));
            g.fillRoundedRectangle(b, cornerRadius);

            // Green chamfer gradient
            juce::ColourGradient chamferGrad(juce::Colour(0xFF7DD49A), b.getX(), b.getY(),
                                             juce::Colour(0xFF266B40), b.getRight(), b.getBottom(), false);
            chamferGrad.addColour(0.3, juce::Colour(0xFF4DB870));
            chamferGrad.addColour(0.65, juce::Colour(0xFF38A058));
            chamferGrad.addColour(0.85, juce::Colour(0xFF48B068));
            g.setGradientFill(chamferGrad);
            g.fillRoundedRectangle(b.reduced(1.0f), cornerRadius - 0.5f);

            // Top face (slightly recessed look)
            auto top = b.reduced(b.getWidth() * 0.055f, b.getHeight() * 0.10f);
            juce::ColourGradient topGrad(juce::Colour(0xFF46AC6A), top.getCentreX(), top.getY(),
                                         juce::Colour(0xFF35904E), top.getCentreX(), top.getBottom(), false);
            g.setGradientFill(topGrad);
            g.fillRoundedRectangle(top, cornerRadius * 0.55f);

            // Top edge highlight
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawRoundedRectangle(top.reduced(0.5f).translated(-0.5f, -0.5f), cornerRadius * 0.5f, 1.0f);
            // Bottom edge shadow
            g.setColour(juce::Colours::black.withAlpha(0.22f));
            g.drawRoundedRectangle(top.reduced(0.5f).translated(0.5f, 0.5f), cornerRadius * 0.5f, 1.0f);

            if (over)
            {
                g.setColour(juce::Colours::white.withAlpha(0.09f));
                g.fillRoundedRectangle(top, cornerRadius * 0.5f);
            }
            if (down)
            {
                g.setColour(juce::Colours::black.withAlpha(0.10f));
                g.fillRoundedRectangle(top, cornerRadius * 0.5f);
            }

            // Label — bold white
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, b.getHeight() * 0.32f), juce::Font::bold)));
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
        else
        {
            // === INACTIVE STATE: metallic silver ===

            // Dark outer bevel border
            g.setColour(juce::Colour(0xFF606062));
            g.fillRoundedRectangle(b, cornerRadius);

            // Silver chamfer gradient (top-left bright, bottom-right dark)
            juce::ColourGradient chamferGrad(juce::Colour(0xFFF4F4F6), b.getX(), b.getY(),
                                             juce::Colour(0xFF787878), b.getRight(), b.getBottom(), false);
            chamferGrad.addColour(0.30, juce::Colour(0xFFCCCCCE));
            chamferGrad.addColour(0.50, juce::Colour(0xFFC0C0C2));
            chamferGrad.addColour(0.68, juce::Colour(0xFF909092));
            chamferGrad.addColour(0.88, juce::Colour(0xFFD0D0D2));
            g.setGradientFill(chamferGrad);
            g.fillRoundedRectangle(b.reduced(1.0f), cornerRadius - 0.5f);

            // Top face
            auto top = b.reduced(b.getWidth() * 0.055f, b.getHeight() * 0.10f);
            juce::ColourGradient topGrad(juce::Colour(0xFFBEBEC0), top.getCentreX(), top.getY(),
                                         juce::Colour(0xFFAEAEB0), top.getCentreX(), top.getBottom(), false);
            g.setGradientFill(topGrad);
            g.fillRoundedRectangle(top, cornerRadius * 0.5f);

            // Edge bevel
            g.setColour(juce::Colours::white.withAlpha(0.48f));
            g.drawRoundedRectangle(top.reduced(0.5f).translated(-0.5f, -0.5f), cornerRadius * 0.5f, 1.0f);
            g.setColour(juce::Colours::black.withAlpha(0.22f));
            g.drawRoundedRectangle(top.reduced(0.5f).translated(0.5f, 0.5f), cornerRadius * 0.5f, 1.0f);

            if (over)
            {
                g.setColour(juce::Colours::white.withAlpha(0.07f));
                g.fillRoundedRectangle(top, cornerRadius * 0.5f);
            }
            if (down)
            {
                g.setColour(juce::Colours::black.withAlpha(0.09f));
                g.fillRoundedRectangle(top, cornerRadius * 0.5f);
            }

            // Label — dark charcoal
            g.setColour(juce::Colour(0xFF303032));
            g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, b.getHeight() * 0.32f), juce::Font::bold)));
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
    }

private:
    juce::String label;
};
