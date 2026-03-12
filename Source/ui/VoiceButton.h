#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VoiceButton : public juce::ToggleButton
{
public:
    enum class IconType { Flame, Hand, Eye };

    VoiceButton(const juce::String& text, IconType type) : label(text), icon(type)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat();
        auto on = getToggleState();
        const float cornerRadius = 16.0f;

        // ── Multi-layer drop shadow ───────────────────────────────
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.fillRoundedRectangle(b.translated(3.0f, 5.0f), cornerRadius);
        g.setColour(juce::Colours::black.withAlpha(0.10f));
        g.fillRoundedRectangle(b.translated(1.5f, 2.5f), cornerRadius);

        // ── Outer bevel (dark green border) ─────────────────────
        g.setColour(juce::Colour(0xFF1A5030));
        g.fillRoundedRectangle(b, cornerRadius);

        // ── Chamfer gradient (simulate conic/angled highlight) ───
        juce::ColourGradient chamferGrad(juce::Colour(0xFF7CD49A), b.getX(), b.getY(),
                                         juce::Colour(0xFF236040), b.getRight(), b.getBottom(), false);
        chamferGrad.addColour(0.12, juce::Colour(0xFF50B872));
        chamferGrad.addColour(0.42, juce::Colour(0xFF2E8050));
        chamferGrad.addColour(0.62, juce::Colour(0xFF3C9860));
        chamferGrad.addColour(0.82, juce::Colour(0xFF44A068));
        chamferGrad.addColour(0.95, juce::Colour(0xFF7CD49A));
        g.setGradientFill(chamferGrad);
        g.fillRoundedRectangle(b.reduced(1.2f), cornerRadius - 1.0f);

        // ── Top face / inner surface ─────────────────────────────
        const float innerInset = 0.055f;
        auto inner = b.reduced(b.getWidth() * innerInset, b.getHeight() * 0.035f);
        const float innerCr = 10.0f;

        juce::ColourGradient innerGrad(juce::Colour(0xFF48A868), inner.getCentreX(), inner.getY(),
                                       juce::Colour(0xFF2A7A48), inner.getCentreX(), inner.getBottom(), false);
        innerGrad.addColour(0.55, juce::Colour(0xFF389858));
        g.setGradientFill(innerGrad);
        g.fillRoundedRectangle(inner, innerCr);

        // Inner face highlight (top edge)
        g.setColour(juce::Colours::white.withAlpha(0.32f));
        g.drawRoundedRectangle(inner.reduced(0.5f).translated(-0.5f, -0.5f), innerCr, 1.0f);
        // Inner face shadow (bottom edge)
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.drawRoundedRectangle(inner.reduced(0.5f).translated(0.5f, 0.5f), innerCr, 1.0f);

        // ── Active state: warm glow overlay + yellow-gold ring ───
        if (on)
        {
            // Warm yellow luminance overlay
            g.setColour(juce::Colour(0x30FFE06E));
            g.fillRoundedRectangle(inner, innerCr);

            // Yellow-gold glow ring (the "sticker pop" look from reference)
            g.setColour(juce::Colour(0xCCFFE050));
            g.drawRoundedRectangle(inner.expanded(1.0f), innerCr + 1.0f, 2.0f);

            // Outer ambient glow
            g.setColour(juce::Colour(0x20FFD840));
            g.fillRoundedRectangle(b.expanded(4.0f), cornerRadius + 4.0f);
        }

        // Hover / press feedback
        if (over)
        {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.fillRoundedRectangle(inner, innerCr);
        }
        if (down)
        {
            g.setColour(juce::Colours::black.withAlpha(0.10f));
            g.fillRoundedRectangle(inner, innerCr);
        }

        // ── Icon ─────────────────────────────────────────────────
        auto iconArea = inner.reduced(inner.getWidth() * 0.13f, inner.getHeight() * 0.09f);
        iconArea = iconArea.removeFromTop(iconArea.getHeight() * 0.75f);
        drawIcon(g, iconArea, on);

        // ── Voice numeral label ───────────────────────────────────
        auto labelArea = inner.removeFromBottom(inner.getHeight() * 0.22f);
        g.setColour(juce::Colours::white.withAlpha(on ? 1.00f : 0.75f));
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(14.0f, inner.getHeight() * 0.07f), juce::Font::bold)));
        g.drawText(label, labelArea.toNearestInt(), juce::Justification::centred);
    }

private:
    void drawIcon(juce::Graphics& g, juce::Rectangle<float> area, bool on)
    {
        auto cx = area.getCentreX();
        auto cy = area.getCentreY();
        auto r = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;

        // Icon colours: warm yellow-orange, darker when off
        auto primaryColour   = on ? juce::Colour(0xFFFFE44C) : juce::Colour(0xFFDDBB38);
        auto secondaryColour = on ? juce::Colour(0xFFFF9820) : juce::Colour(0xFFCC8018);
        auto darkColour      = on ? juce::Colour(0xFFCC6010) : juce::Colour(0xFFAA5010);

        if (icon == IconType::Flame)
        {
            // === STARBURST / SPARK (4-pointed sharp star with radiating lines) ===
            juce::Path star;
            const int numPoints = 8;
            for (int i = 0; i < numPoints * 2; ++i)
            {
                float angle = i * juce::MathConstants<float>::pi / numPoints
                              - juce::MathConstants<float>::halfPi;
                float rad = (i % 2 == 0) ? r * 0.90f : r * 0.28f;
                float px = cx + std::cos(angle) * rad;
                float py = cy + std::sin(angle) * rad;
                if (i == 0) star.startNewSubPath(px, py);
                else        star.lineTo(px, py);
            }
            star.closeSubPath();

            // Shadow
            g.setColour(darkColour.withAlpha(0.45f));
            g.fillPath(star, juce::AffineTransform::translation(1.5f, 2.0f));

            // Body gradient
            juce::ColourGradient starGrad(primaryColour, cx - r * 0.3f, cy - r * 0.6f,
                                          secondaryColour, cx + r * 0.3f, cy + r * 0.6f, false);
            starGrad.addColour(0.5, juce::Colour(0xFFFFBB20));
            g.setGradientFill(starGrad);
            g.fillPath(star);

            // Centre white glow
            g.setColour(juce::Colours::white.withAlpha(on ? 0.90f : 0.55f));
            g.fillEllipse(cx - r * 0.18f, cy - r * 0.18f, r * 0.36f, r * 0.36f);

            // Radiating cross lines
            g.setColour(primaryColour.withAlpha(0.55f));
            {
                juce::Path lines;
                for (int i = 0; i < 4; ++i)
                {
                    float ang = i * juce::MathConstants<float>::halfPi;
                    lines.startNewSubPath(cx + std::cos(ang) * r * 0.22f,
                                         cy + std::sin(ang) * r * 0.22f);
                    lines.lineTo(cx + std::cos(ang) * r * 1.05f,
                                 cy + std::sin(ang) * r * 1.05f);
                }
                g.strokePath(lines, juce::PathStrokeType(juce::jmax(1.0f, r * 0.05f)));
            }
        }
        else if (icon == IconType::Hand)
        {
            // === HAND / GLOVE (open palm) ===
            g.setColour(primaryColour);

            auto handW = r * 1.10f;
            auto handH = r * 1.35f;
            auto hx = cx - handW * 0.50f;
            auto hy = cy - handH * 0.44f;

            // Four fingers
            const float fingerW  = handW * 0.18f;
            const float fingerGap = handW * 0.04f;
            const float fingerTotalW = fingerW * 4 + fingerGap * 3;
            const float fingerStartX = cx - fingerTotalW * 0.5f;
            const float fingerHeights[4] = { 0.42f, 0.50f, 0.48f, 0.38f };
            for (int i = 0; i < 4; ++i)
            {
                float fx = fingerStartX + i * (fingerW + fingerGap);
                float fh = handH * fingerHeights[i];
                juce::Path finger;
                finger.addRoundedRectangle(fx, hy, fingerW, fh + handH * 0.10f, fingerW * 0.45f);
                g.fillPath(finger);
            }

            // Thumb
            {
                juce::Path thumb;
                float tx = hx - handW * 0.04f;
                float ty = hy + handH * 0.32f;
                thumb.addRoundedRectangle(tx, ty, handW * 0.16f, handH * 0.28f, handW * 0.06f);
                g.fillPath(thumb, juce::AffineTransform::rotation(-0.25f, tx + handW * 0.08f, ty + handH * 0.14f));
            }

            // Palm
            {
                juce::Path palm;
                float py = hy + handH * 0.40f;
                palm.addRoundedRectangle(hx - handW * 0.04f, py, handW * 1.08f, handH * 0.48f, handW * 0.18f);
                g.fillPath(palm);
            }

            // Knuckle lines
            g.setColour(darkColour.withAlpha(0.40f));
            for (int i = 0; i < 3; ++i)
            {
                float lx = fingerStartX + fingerW * 0.1f;
                float ly = hy + handH * (0.16f + i * 0.08f);
                g.drawRect(lx, ly, fingerTotalW * 0.88f, 1.5f);
            }

            // Palm highlight
            g.setColour(juce::Colours::white.withAlpha(on ? 0.30f : 0.16f));
            g.fillEllipse(hx + handW * 0.18f, hy + handH * 0.48f, handW * 0.40f, handH * 0.16f);
        }
        else // IconType::Eye — spiky blob / sea-urchin explosion
        {
            // === SPIKY BLOB (matches 3rd sticker in reference) ===
            const int numSpikes = 12;
            juce::Path blob;
            for (int i = 0; i < numSpikes * 2; ++i)
            {
                float angle = i * juce::MathConstants<float>::pi / numSpikes
                              - juce::MathConstants<float>::halfPi;
                float rad = (i % 2 == 0) ? r * 0.88f : r * 0.42f;
                if (i % 4 == 0) rad = r * 0.95f;
                float px = cx + std::cos(angle) * rad;
                float py = cy + std::sin(angle) * rad;
                if (i == 0) blob.startNewSubPath(px, py);
                else        blob.lineTo(px, py);
            }
            blob.closeSubPath();

            // Shadow
            g.setColour(darkColour.withAlpha(0.40f));
            g.fillPath(blob, juce::AffineTransform::translation(1.5f, 2.0f));

            // Body
            juce::ColourGradient blobGrad(primaryColour, cx - r * 0.4f, cy - r * 0.7f,
                                          secondaryColour, cx + r * 0.4f, cy + r * 0.6f, false);
            blobGrad.addColour(0.45, juce::Colour(0xFFFFAA18));
            g.setGradientFill(blobGrad);
            g.fillPath(blob);

            // Outline
            g.setColour(darkColour.withAlpha(0.45f));
            g.strokePath(blob, juce::PathStrokeType(juce::jmax(1.0f, r * 0.04f)));

            // Bright core
            juce::ColourGradient coreGrad(juce::Colours::white.withAlpha(on ? 0.90f : 0.60f),
                                          cx, cy - r * 0.14f,
                                          primaryColour.withAlpha(0.0f),
                                          cx, cy + r * 0.30f, false);
            g.setGradientFill(coreGrad);
            g.fillEllipse(cx - r * 0.28f, cy - r * 0.30f, r * 0.56f, r * 0.56f);
        }
    }

    juce::String label;
    IconType icon;
};
