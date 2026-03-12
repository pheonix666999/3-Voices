#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_video/juce_video.h>
#include "PresetBar.h"

class ScreenPanel : public juce::Component, private juce::Timer
{
public:
    ScreenPanel()
    {
        addAndMakeVisible(presetBar);

        const juce::File videoFile("H:/PROJECTS/Fiverr/3 Voices/Greyland Audio Animation.mp4");
        if (videoFile.existsAsFile())
        {
            videoComponent = std::make_unique<juce::VideoComponent>(false);
            videoComponent->setAudioVolume(0.0f);
            if (videoComponent->load(videoFile).wasOk())
            {
                videoComponent->onPlaybackStopped = [this]
                {
                    if (videoComponent != nullptr && hasVideo)
                    {
                        videoComponent->setPlayPosition(0.0);
                        videoComponent->play();
                    }
                };

                addAndMakeVisible(*videoComponent);
                hasVideo = true;
                startTimerHz(20);
                return;
            }

            videoComponent.reset();
        }

        // Fallback to image frames if the video file is not available.
        juce::File animDir("H:/PROJECTS/Fiverr/3 Voices/Assets/Anim");
        if (animDir.isDirectory())
        {
            auto files = animDir.findChildFiles(juce::File::findFiles, false, "*.jpg");
            files.sort();
            for (auto& f : files)
            {
                auto img = juce::ImageCache::getFromFile(f);
                if (img.isValid())
                    animFrames.add(img);
            }
        }

        if (animFrames.size() > 0)
            startTimerHz(24);
    }

    void timerCallback() override
    {
        if (hasVideo && videoComponent != nullptr)
        {
            if (!videoStarted)
            {
                videoComponent->play();
                videoStarted = true;
                return;
            }

            // Keep the MP4 looping.
            if (!videoComponent->isPlaying())
            {
                videoComponent->setPlayPosition(0.0);
                videoComponent->play();
            }

            return;
        }

        if (animFrames.size() > 0)
        {
            animFrameIndex = (animFrameIndex + 1) % animFrames.size();
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto cornerRadius = juce::jmin(b.getWidth(), b.getHeight()) * 0.09f;

        juce::ColourGradient bezelGrad(juce::Colour(0xFFC4CDD1), b.getX(), b.getY(),
                                       juce::Colour(0xFFCFD7DB), b.getRight(), b.getBottom(), false);
        bezelGrad.addColour(0.1, juce::Colour(0xFFAAB6BF));
        bezelGrad.addColour(0.25, juce::Colour(0xFFA0AFB5));
        bezelGrad.addColour(0.4, juce::Colour(0xFF9FAEB4));
        bezelGrad.addColour(0.5, juce::Colour(0xFFB0BCC1));
        bezelGrad.addColour(0.6, juce::Colour(0xFFBAC1CA));
        bezelGrad.addColour(0.65, juce::Colour(0xFFE3E4E8));
        bezelGrad.addColour(0.85, juce::Colour(0xFFE2EDF0));
        g.setGradientFill(bezelGrad);
        g.fillRoundedRectangle(b, cornerRadius);

        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawRoundedRectangle(b.reduced(1.0f).translated(-1.0f, -1.0f), cornerRadius, 1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(b.reduced(1.0f).translated(1.0f, 1.0f), cornerRadius, 1.5f);

        {
            juce::ColourGradient insetGrad(juce::Colours::black.withAlpha(0.25f), b.getX(), b.getY(),
                                           juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(), true);
            g.setGradientFill(insetGrad);
            g.fillRoundedRectangle(b, cornerRadius);
        }

        auto padding = juce::jmax(8.0f, b.getWidth() * 0.02f);
        auto innerRect = b.reduced(padding);

        auto headerHeight = innerRect.getHeight() * 0.235f;
        auto headerBounds = innerRect.removeFromTop(headerHeight);
        auto headerCornerTop = juce::jmin(40.0f, headerBounds.getWidth() * 0.08f);

        g.setColour(juce::Colour(0xFF0F0F0F));
        g.fillRoundedRectangle(headerBounds, headerCornerTop);
        g.setColour(juce::Colour(0xFF818F96));
        g.drawRoundedRectangle(headerBounds, headerCornerTop, 2.0f);

        g.setColour(juce::Colour(0xFF43A464).withAlpha(0.35f));
        g.drawRect(headerBounds.reduced(headerBounds.getWidth() * 0.06f, 0.0f)
                     .withHeight(1.5f)
                     .translated(0.0f, headerBounds.getHeight() * 0.5f));
        g.drawRect(headerBounds.reduced(0.0f, headerBounds.getHeight() * 0.12f)
                     .withWidth(1.5f)
                     .translated(headerBounds.getWidth() * 0.5f, 0.0f));

        auto gap = juce::jmax(3.0f, b.getHeight() * 0.005f);
        innerRect.removeFromTop(gap);
        auto bodyBounds = innerRect;
        auto bodyCornerBottom = juce::jmin(40.0f, bodyBounds.getWidth() * 0.08f);

        g.setColour(juce::Colour(0xFF0F0F0F));
        g.fillRoundedRectangle(bodyBounds, bodyCornerBottom);

        // Fallback frame animation if MP4 is unavailable.
        if (!hasVideo && animFrames.size() > 0)
        {
            auto currentFrame = animFrames[animFrameIndex];
            g.saveState();
            juce::Path p;
            p.addRoundedRectangle(bodyBounds, bodyCornerBottom);
            g.reduceClipRegion(p);
            auto placement = juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination;
            g.drawImage(currentFrame, bodyBounds, placement);
            g.restoreState();
        }

        g.setColour(juce::Colour(0xFF818F96));
        g.drawRoundedRectangle(bodyBounds, bodyCornerBottom, 2.0f);

        g.setColour(juce::Colour(0xFF43A464).withAlpha(0.25f));
        g.drawRect(bodyBounds.reduced(bodyBounds.getWidth() * 0.04f, 0.0f)
                     .withHeight(1.5f)
                     .translated(0.0f, bodyBounds.getHeight() * 0.5f));
        g.drawRect(bodyBounds.reduced(0.0f, bodyBounds.getHeight() * 0.02f)
                     .withWidth(1.5f)
                     .translated(bodyBounds.getWidth() * 0.5f, 0.0f));
    }

    void resized() override
    {
        auto b = getLocalBounds().toFloat();
        auto padding = juce::jmax(8.0f, b.getWidth() * 0.02f);
        auto innerRect = b.reduced(padding);
        auto headerHeight = innerRect.getHeight() * 0.235f;
        auto headerBounds = innerRect.removeFromTop((float) headerHeight);

        auto barBounds = headerBounds.reduced(headerBounds.getWidth() * 0.06f,
                                              headerBounds.getHeight() * 0.18f);
        presetBar.setBounds(barBounds.toNearestInt());

        auto gap = juce::jmax(3.0f, b.getHeight() * 0.005f);
        innerRect.removeFromTop(gap);
        auto bodyBounds = innerRect.reduced(innerRect.getWidth() * 0.02f, innerRect.getHeight() * 0.02f);
        if (videoComponent != nullptr)
            videoComponent->setBounds(bodyBounds.toNearestInt());
    }

    PresetBar presetBar;

private:
    juce::Array<juce::Image> animFrames;
    int animFrameIndex = 0;

    std::unique_ptr<juce::VideoComponent> videoComponent;
    bool hasVideo = false;
    bool videoStarted = false;
};
