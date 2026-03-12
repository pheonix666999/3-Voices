#include "UnisonLookAndFeel.h"

namespace
{
    // ---------- helper: metallic circle ----------
    void paintMetallicCircle(juce::Graphics& g,
                             juce::Rectangle<float> bounds,
                             juce::Colour top,
                             juce::Colour bottom,
                             float borderThickness,
                             juce::Colour borderColour)
    {
        g.setColour(borderColour);
        g.fillEllipse(bounds);
        auto inner = bounds.reduced(borderThickness);
        juce::ColourGradient grad(top, inner.getCentreX(), inner.getY(),
                                  bottom, inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill(grad);
        g.fillEllipse(inner);
    }
}

UnisonLookAndFeel::UnisonLookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId,          juce::Colour(0xF0111111));
    setColour(juce::PopupMenu::textColourId,                juce::Colour(0xFFD0D0D0));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xCC3A7F50));
    setColour(juce::PopupMenu::highlightedTextColourId,     juce::Colours::white);
}

// ============================================================
//  ROTARY SLIDER  — skeuomorphic silver / green knob
// ============================================================
void UnisonLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    auto area = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    const auto diameter = juce::jmin(area.getWidth(), area.getHeight());
    auto knobBounds = area.withSizeKeepingCentre(diameter, diameter);
    const auto centre = knobBounds.getCentre();

    const bool isMixKnob = (slider.getMinimum() >= -0.01 && slider.getMaximum() <= 100.01
                            && diameter > 120.0f);

    // Multi-layer cast shadow to mimic Figma stacked shadows.
    auto shadowA = knobBounds.reduced(diameter * 0.27f).translated(diameter * 0.18f, diameter * 0.14f);
    auto shadowB = knobBounds.reduced(diameter * 0.34f).translated(diameter * 0.16f, diameter * 0.15f);
    g.setColour(juce::Colours::black.withAlpha(0.10f));
    g.fillEllipse(shadowA.expanded(diameter * 0.035f, diameter * 0.016f));
    g.setColour(juce::Colours::black.withAlpha(0.06f));
    g.fillEllipse(shadowB.expanded(diameter * 0.025f, diameter * 0.012f));

    auto base = knobBounds.reduced(diameter * 0.025f);
    juce::ColourGradient baseGrad(juce::Colour(0xFFBBBBBD), base.getCentreX(), base.getY(),
                                  juce::Colour(0xFFB0B0B2), base.getCentreX(), base.getBottom(), false);
    g.setGradientFill(baseGrad);
    g.fillEllipse(base);
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawEllipse(base, juce::jmax(1.0f, diameter * 0.005f));

    // Rim highlights.
    g.setColour(juce::Colours::white.withAlpha(0.36f));
    g.drawEllipse(base.reduced(1.5f), juce::jmax(1.0f, diameter * 0.006f));
    g.setColour(juce::Colours::black.withAlpha(0.14f));
    g.drawEllipse(base.reduced(3.0f), juce::jmax(0.8f, diameter * 0.004f));

    // Knurled ring/cap body - exact CSS-like proportions from 304px base.
    auto knurled = base.withSizeKeepingCentre(base.getWidth() * 0.609f, base.getHeight() * 0.609f);
    if (isMixKnob)
    {
        juce::ColourGradient knurlGrad(juce::Colour(0xFF43A464), knurled.getX(), knurled.getY(),
                                       juce::Colour(0xFF3D945B), knurled.getRight(), knurled.getBottom(), false);
        g.setGradientFill(knurlGrad);
    }
    else
    {
        juce::ColourGradient knurlGrad(juce::Colour(0xFFFCFBFE), knurled.getX(), knurled.getY(),
                                       juce::Colour(0xFFCDCBCE), knurled.getRight(), knurled.getBottom(), false);
        g.setGradientFill(knurlGrad);
    }
    g.fillEllipse(knurled);

    // Notches around the knurled edge.
    const int notches = 72;
    const float r0 = knurled.getWidth() * 0.455f;
    const float r1 = knurled.getWidth() * 0.50f;
    g.setColour(isMixKnob ? juce::Colour(0xFF2A7C49).withAlpha(0.35f)
                          : juce::Colour(0xFF7F7F82).withAlpha(0.40f));
    for (int i = 0; i < notches; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float)i / (float)notches;
        const float x0 = knurled.getCentreX() + std::cos(a) * r0;
        const float y0 = knurled.getCentreY() + std::sin(a) * r0;
        const float x1 = knurled.getCentreX() + std::cos(a) * r1;
        const float y1 = knurled.getCentreY() + std::sin(a) * r1;
        g.drawLine(x0, y0, x1, y1, juce::jmax(0.8f, diameter * 0.0035f));
    }

    auto topOuter = base.withSizeKeepingCentre(base.getWidth() * 0.586f, base.getHeight() * 0.586f);
    if (isMixKnob)
    {
        juce::ColourGradient topOuterGrad(juce::Colour(0xFF43A464), topOuter.getX(), topOuter.getY(),
                                          juce::Colour(0xFF3A9559), topOuter.getRight(), topOuter.getBottom(), false);
        g.setGradientFill(topOuterGrad);
    }
    else
    {
        juce::ColourGradient topOuterGrad(juce::Colour(0xFFA2A0A1), topOuter.getX(), topOuter.getY(),
                                          juce::Colour(0xFFB0B0B0), topOuter.getRight(), topOuter.getBottom(), false);
        g.setGradientFill(topOuterGrad);
    }
    g.fillEllipse(topOuter);
    g.setColour(juce::Colours::white.withAlpha(0.40f));
    g.drawEllipse(topOuter.reduced(1.0f).translated(-0.7f, -0.7f), juce::jmax(0.8f, diameter * 0.0038f));

    auto topCap = base.withSizeKeepingCentre(base.getWidth() * 0.531f, base.getHeight() * 0.531f);
    if (isMixKnob)
    {
        juce::ColourGradient topCapGrad(juce::Colour(0xFF4CAE6E), topCap.getX(), topCap.getY(),
                                        juce::Colour(0xFF3C9C5D), topCap.getRight(), topCap.getBottom(), false);
        g.setGradientFill(topCapGrad);
    }
    else
    {
        juce::ColourGradient topCapGrad(juce::Colour(0xFFA5A5A5), topCap.getX(), topCap.getY(),
                                        juce::Colour(0xFFCACACA), topCap.getRight(), topCap.getBottom(), false);
        g.setGradientFill(topCapGrad);
    }
    g.fillEllipse(topCap);
    g.setColour(juce::Colours::white.withAlpha(0.40f));
    g.drawEllipse(topCap.reduced(1.0f).translated(-0.7f, -0.7f), juce::jmax(0.8f, diameter * 0.0035f));
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawEllipse(topCap.reduced(1.5f).translated(0.7f, 0.7f), juce::jmax(0.8f, diameter * 0.0035f));

    // Indicator dot.
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float indicatorRadius = base.getWidth() * 0.40f;
    const float dotSize = juce::jmax(4.0f, base.getWidth() * 0.052f);
    const float dotX = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * indicatorRadius - dotSize * 0.5f;
    const float dotY = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * indicatorRadius - dotSize * 0.5f;
    g.setColour(isMixKnob ? juce::Colour(0xFF1E4B2D) : juce::Colour(0xFF606060));
    g.fillEllipse(dotX, dotY, dotSize, dotSize);
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    g.drawEllipse(dotX, dotY, dotSize, dotSize, 1.0f);
}

// ============================================================
//  LINEAR SLIDER  — Fader thumb & track
// ============================================================
void UnisonLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float /*minSliderPos*/,
                                         float /*maxSliderPos*/,
                                         juce::Slider::SliderStyle style,
                                         juce::Slider& /*slider*/)
{
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);

    if (style == juce::Slider::LinearHorizontal)
    {
        // Width slider: draw a clean rounded slot and constrain thumb inside end caps.
        const float thumbD = juce::jmax(20.0f, b.getHeight() * 0.46f);
        const float railH = juce::jmax(12.0f, b.getHeight() * 0.20f);
        const float railX = b.getX() + b.getWidth() * 0.07f;
        const float railW = b.getWidth() * 0.70f;
        const float railY = b.getY() + b.getHeight() * 0.35f;
        auto rail = juce::Rectangle<float>(railX, railY, railW, railH);
        const float railCorner = railH * 0.5f;

        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(rail.translated(1.0f, 1.5f), railCorner);
        g.setColour(juce::Colour(0xFF0F1012));
        g.fillRoundedRectangle(rail, railCorner);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(rail.reduced(0.5f), railCorner, 1.0f);

        const float minCx = rail.getX() + railCorner;
        const float maxCx = rail.getRight() - railCorner;
        const float cx = juce::jlimit(minCx, maxCx, sliderPos);
        auto thumbBounds = juce::Rectangle<float>(cx - thumbD * 0.5f,
                                                  rail.getCentreY() - thumbD * 0.5f,
                                                  thumbD, thumbD);
        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillEllipse(thumbBounds.translated(2.0f, 3.0f).expanded(1.0f));

        // Body gradient
        juce::ColourGradient thumbGrad(juce::Colour(0xFFC8C8CA), thumbBounds.getCentreX(), thumbBounds.getY(),
                                       juce::Colour(0xFFAAAAAC), thumbBounds.getCentreX(), thumbBounds.getBottom(), false);
        thumbGrad.addColour(0.5, juce::Colour(0xFFBCBCBE));
        g.setGradientFill(thumbGrad);
        g.fillEllipse(thumbBounds);

        // Outer chamfer
        g.setColour(juce::Colour(0xFF606062));
        g.drawEllipse(thumbBounds.reduced(0.5f), 1.0f);

        // Top surface specular
        auto topSurf = thumbBounds.reduced(thumbD * 0.18f);
        juce::ColourGradient surfGrad(juce::Colour(0xFFD2D2D4), topSurf.getCentreX(), topSurf.getY(),
                                      juce::Colour(0xFFBBBBBD), topSurf.getCentreX(), topSurf.getBottom(), false);
        g.setGradientFill(surfGrad);
        g.fillEllipse(topSurf);
        g.setColour(juce::Colours::white.withAlpha(0.50f));
        g.drawEllipse(topSurf.reduced(1.0f).translated(-0.5f, -0.5f), 1.0f);

        return;
    }

    // Vertical sliders use the baked rail in the background PNG. Draw thumb only.

    // ── Thumb ────────────────────────────────────────────────
    const bool isSmallFader = (width < 60);

    if (isSmallFader)
    {
        // Distortion slider — small metallic pill
        const float thumbSize = juce::jmax(18.0f, b.getWidth() * 1.20f);
        const float endPad = thumbSize * 0.34f;
        const float cy = juce::jlimit(b.getY() + endPad, b.getBottom() - endPad, sliderPos);
        auto thumbBounds = juce::Rectangle<float>(b.getCentreX() - thumbSize * 0.5f,
                                                  cy - thumbSize * 0.5f,
                                                  thumbSize, thumbSize);
        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.fillEllipse(thumbBounds.translated(1.5f, 2.5f).expanded(1.0f));

        // Chamfer border
        g.setColour(juce::Colour(0xFF565658));
        g.fillEllipse(thumbBounds);

        // Body
        juce::ColourGradient bodyGrad(juce::Colour(0xFFC8C8CA), thumbBounds.getCentreX(), thumbBounds.getY(),
                                      juce::Colour(0xFFAAAAAC), thumbBounds.getCentreX(), thumbBounds.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(thumbBounds.reduced(1.5f));

        // Top surface
        auto topThumb = thumbBounds.reduced(thumbSize * 0.16f);
        juce::ColourGradient tGrad(juce::Colour(0xFFCECED0), topThumb.getCentreX(), topThumb.getY(),
                                   juce::Colour(0xFFB4B4B6), topThumb.getCentreX(), topThumb.getBottom(), false);
        g.setGradientFill(tGrad);
        g.fillEllipse(topThumb);
        g.setColour(juce::Colours::white.withAlpha(0.48f));
        g.drawEllipse(topThumb.reduced(1.0f).translated(-0.5f, -0.5f), 1.0f);
    }
    else
    {
        // Input/Output gain slider — large green metallic puck
        const float thumbSize = juce::jmax(28.0f, b.getWidth() * 1.55f);
        const float endPad = thumbSize * 0.44f;
        const float cy = juce::jlimit(b.getY() + endPad, b.getBottom() - endPad, sliderPos);
        auto thumbBounds = juce::Rectangle<float>(b.getCentreX() - thumbSize * 0.5f,
                                                  cy - thumbSize * 0.5f,
                                                  thumbSize, thumbSize);
        // Layered shadow
        g.setColour(juce::Colours::black.withAlpha(0.28f));
        g.fillEllipse(thumbBounds.translated(3.0f, 5.0f).expanded(3.0f));
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.fillEllipse(thumbBounds.translated(1.5f, 2.5f).expanded(1.5f));

        // Outer chamfer (dark border)
        g.setColour(juce::Colour(0xFF1E6640));
        g.fillEllipse(thumbBounds);

        // Green body gradient
        juce::ColourGradient chamferGrad(juce::Colour(0xFF8ED4A6), thumbBounds.getX(), thumbBounds.getY(),
                                         juce::Colour(0xFF256B40), thumbBounds.getRight(), thumbBounds.getBottom(), false);
        chamferGrad.addColour(0.25, juce::Colour(0xFF52B876));
        chamferGrad.addColour(0.55, juce::Colour(0xFF3EA062));
        chamferGrad.addColour(0.80, juce::Colour(0xFF3A9A5C));
        g.setGradientFill(chamferGrad);
        g.fillEllipse(thumbBounds.reduced(1.5f));

        // Top surface
        auto topSurface = thumbBounds.reduced(thumbSize * 0.16f);
        juce::ColourGradient surfGrad(juce::Colour(0xFF4AB870), topSurface.getCentreX(), topSurface.getY(),
                                      juce::Colour(0xFF318050), topSurface.getCentreX(), topSurface.getBottom(), false);
        g.setGradientFill(surfGrad);
        g.fillEllipse(topSurface);

        // Specular highlight arc
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        {
            juce::Path hl;
            hl.addArc(topSurface.getX(), topSurface.getY(),
                      topSurface.getWidth(), topSurface.getHeight(),
                      -juce::MathConstants<float>::pi * 0.75f,
                      juce::MathConstants<float>::pi * 0.05f, true);
            g.strokePath(hl, juce::PathStrokeType(1.8f));
        }
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        {
            juce::Path sh;
            sh.addArc(topSurface.getX(), topSurface.getY(),
                      topSurface.getWidth(), topSurface.getHeight(),
                      juce::MathConstants<float>::pi * 0.25f,
                      juce::MathConstants<float>::pi * 0.95f, true);
            g.strokePath(sh, juce::PathStrokeType(1.8f));
        }

        // Centre dot
        const float dotR = topSurface.getWidth() * 0.10f;
        g.setColour(juce::Colour(0xFF1A5A35));
        g.fillEllipse(topSurface.getCentreX() - dotR, topSurface.getCentreY() - dotR, dotR * 2.0f, dotR * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.30f));
        g.fillEllipse(topSurface.getCentreX() - dotR * 0.4f, topSurface.getCentreY() - dotR * 0.5f, dotR * 0.7f, dotR * 0.5f);
    }
}

// ============================================================
//  BUTTON BACKGROUND  — generic toggle / text buttons
// ============================================================
void UnisonLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                             juce::Button& /*button*/,
                                             const juce::Colour& base,
                                             bool over,
                                             bool down)
{
    auto b = g.getClipBounds().toFloat();
    const float cr = 7.0f;
    auto fill = base;
    if (over) fill = fill.brighter(0.07f);
    if (down) fill = fill.darker(0.12f);

    // Drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(b.translated(1.5f, 2.0f), cr);

    // Body
    g.setColour(fill);
    g.fillRoundedRectangle(b, cr);

    // Dark border
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(b.reduced(0.5f), cr, 1.0f);

    // Top highlight
    g.setColour(juce::Colours::white.withAlpha(0.28f));
    {
        juce::Path topEdge;
        topEdge.addRoundedRectangle(b.getX() + 4.0f, b.getY() + 1.0f,
                                     b.getWidth() - 8.0f, 2.5f, 1.2f);
        g.fillPath(topEdge);
    }
}

// ============================================================
//  POPUP MENU
// ============================================================
void UnisonLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    juce::ColourGradient bg(juce::Colour(0xF5101010), b.getCentreX(), b.getY(),
                             juce::Colour(0xF5181818), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(juce::Colour(0x88404040));
    g.drawRoundedRectangle(b.reduced(0.8f), 10.0f, 1.0f);
}

void UnisonLookAndFeel::drawPopupMenuItem(juce::Graphics& g,
                                          const juce::Rectangle<int>& area,
                                          bool isSeparator,
                                          bool isActive,
                                          bool isHighlighted,
                                          bool isTicked,
                                          bool /*hasSubMenu*/,
                                          const juce::String& text,
                                          const juce::String& /*shortcut*/,
                                          const juce::Drawable* /*icon*/,
                                          const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(juce::Colour(0x55808080));
        g.fillRect(area.reduced(10, area.getHeight() / 2 - 1).withHeight(1));
        return;
    }

    auto a = area.toFloat().reduced(5.0f, 2.0f);
    if (isHighlighted)
    {
        juce::ColourGradient hlGrad(juce::Colour(0xCC3A7F50), a.getX(), a.getY(),
                                    juce::Colour(0xAA2D6440), a.getRight(), a.getBottom(), false);
        g.setGradientFill(hlGrad);
        g.fillRoundedRectangle(a, 6.0f);
        g.setColour(juce::Colour(0x6643A464));
        g.drawRoundedRectangle(a.reduced(0.5f), 6.0f, 1.0f);
    }

    juce::Colour c = textColour != nullptr ? *textColour : findColour(juce::PopupMenu::textColourId);
    if (!isActive) c = c.withAlpha(0.40f);
    g.setColour(c);
    g.setFont(getPopupMenuFont());
    g.drawText(text, area.reduced(14, 0), juce::Justification::centredLeft, true);

    if (isTicked)
    {
        g.setColour(juce::Colour(0xFF6ACE8C));
        g.fillEllipse((float)(area.getRight() - 18), (float)(area.getCentreY() - 3), 6.0f, 6.0f);
    }
}

juce::Font UnisonLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(15.0f, juce::Font::bold));
}
