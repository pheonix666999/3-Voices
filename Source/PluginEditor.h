#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_video/juce_video.h>

#include "PluginProcessor.h"
#include "ui/UnisonLookAndFeel.h"
#include "ui/InvisibleLookAndFeel.h"
#include "ui/PresetMenuOverlay.h"

// Slider with paint() suppressed — used as invisible value holder only.
// Visual rendering is done by the parent editor via paintOverChildren().
// Mouse events are forwarded from the parent editor manually.
struct SilentSlider : public juce::Slider
{
    void paint(juce::Graphics&) override {}

    // Force permanently invisible — SliderAttachment or JUCE internals
    // may call setVisible(true); this overrides it immediately every time.
    void visibilityChanged() override
    {
        if (isVisible()) setVisible(false);
    }
};

class ThreeVoicesAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::AudioProcessorValueTreeState::Listener,
                                        private juce::Timer
{
public:
    explicit ThreeVoicesAudioProcessorEditor(ThreeVoicesAudioProcessor&);
    ~ThreeVoicesAudioProcessorEditor() override;

    void paint            (juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized          () override;

    // Mouse forwarding to invisible linear sliders
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    static constexpr int designW = 3366;
    static constexpr int designH = 1945;

    struct RowControls
    {
        juce::Slider speedKnob;
        juce::Slider delayKnob;
        juce::Slider depthKnob;
        SilentSlider distortionSlider;

        juce::ToggleButton voiceButton;
        juce::ToggleButton bitButton;
        juce::ToggleButton tubeButton;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distortionAttachment;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> voiceAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bitAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tubeAttachment;
    };

    void initialiseControls();
    void initialiseAttachments();
    void initialiseSideFaderArt();
    void buildPresetPreviewLibrary();

    juce::Rectangle<int> scaleRect(const juce::Rectangle<int>& ref) const;
    void loadImages();
    juce::Image loadImageByName(const juce::String& fileName) const;
    juce::File findAssetFile(const juce::String& fileName) const;
    void initialiseScreenAnimation();

    bool anyToggleActive() const;
    juce::String getCurrentPresetName() const;
    void drawPresetDisplay(juce::Graphics& g);
    void drawToggleOverlays(juce::Graphics& g);
    void drawSideFaderHandles(juce::Graphics& g);
    void drawDistortionSliderHandles(juce::Graphics& g);
    void drawWidthSliderOverlay(juce::Graphics& g);
    void drawScreenFallbackAnimation(juce::Graphics& g);

    void drawRotaryKnobOverlays(juce::Graphics& g);
    void drawSingleRotaryKnobOverlay(juce::Graphics& g,
                                     const juce::Slider& slider,
                                     const juce::Rectangle<int>& refBounds,
                                     bool greenCap,
                                     float sizeScale = 1.0f);

    void updateScreenAnimationPlayback();
    bool advanceFallbackAnimationFrame();

    void openPresetOverlay();
    void closePresetOverlay();
    void onOverlayPresetSelected(int categoryIndex, int presetIndex);
    void stepPreset(int delta);

    void timerCallback() override;
    void parameterChanged(const juce::String&, float) override;

    // Returns the linear slider whose bounds contain localPos, or nullptr.
    juce::Slider* findLinearSliderAt(juce::Point<int> localPos);

    ThreeVoicesAudioProcessor& audioProcessor;
    UnisonLookAndFeel unisonLookAndFeel;
    InvisibleLookAndFeel invisibleLookAndFeel;

    juce::Image bgStandard;
    juce::Image bgButtonsOn;

    juce::Image menuCategories;
    juce::Image menuClassicMod;
    juce::Image menuGuitar;
    juce::Image menuKeysSynth;
    juce::Image menuBass;
    juce::Image menuDrums;
    juce::Image menuVocals;
    juce::Image leftSideFaderSprite;
    juce::Image rightSideFaderSprite;
    juce::Image sideFaderBottomCleanPatch;
    juce::Image sideFaderThumb;       // grip image drawn as the fader thumb

    std::array<RowControls, 3> rows;

    SilentSlider widthSlider;
    juce::Slider mixKnob;
    SilentSlider inputGainSlider;
    SilentSlider outputGainSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    juce::TextButton presetButton;
    juce::TextButton previousPresetButton;
    juce::TextButton nextPresetButton;
    std::unique_ptr<PresetMenuOverlay> presetOverlay;
    std::unique_ptr<juce::VideoComponent> screenVideo;
    juce::Array<juce::Image> animationFrames;
    juce::Array<juce::Image> presetPreviewImages;
    int currentAnimationFrame = 0;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::Listener>> ownedListeners;

    juce::CriticalSection stateLock;
    juce::String cachedPresetName;

    // Mouse forwarding state
    juce::Slider* activeDragSlider = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeVoicesAudioProcessorEditor)
};
