#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr std::array<const char*, 3> kVoiceIds      { "voice1", "voice2", "voice3" };
constexpr std::array<const char*, 3> kBitIds        { "dist_bit_1", "dist_bit_2", "dist_bit_3" };
constexpr std::array<const char*, 3> kTubeIds       { "dist_tube_1", "dist_tube_2", "dist_tube_3" };
constexpr std::array<const char*, 3> kSpeedIds      { "voice1Speed", "voice2Speed", "voice3Speed" };
constexpr std::array<const char*, 3> kDelayIds      { "voice1DelayTime", "voice2DelayTime", "voice3DelayTime" };
constexpr std::array<const char*, 3> kDepthIds      { "voice1Depth", "voice2Depth", "voice3Depth" };
constexpr std::array<const char*, 3> kDistortionIds { "voice1Distortion", "voice2Distortion", "voice3Distortion" };

struct RectAdjust
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct PointAdjust
{
    float x = 0.0f;
    float y = 0.0f;
};

static juce::Rectangle<int> applyRectAdjust(juce::Rectangle<int> r, const RectAdjust& a)
{
    r.setPosition(r.getX() + a.x, r.getY() + a.y);
    r.setSize(r.getWidth() + a.w, r.getHeight() + a.h);
    return r;
}

const juce::Rectangle<int> kRightFaderBottomBlobCoverRef { 3050, 1200, 202, 772 };
const juce::Rectangle<int> kScreenBodyRef { 2272, 724,  620, 637 };
const juce::Rectangle<int> kVideoRef      { 2330, 750,  580, 590 };
const juce::Rectangle<int> kPresetTextRef { 2458, 566, 432, 58 };
const juce::Rectangle<int> kPresetOpenRef { 2438, 533, 470, 126 };
const juce::Rectangle<int> kPresetPrevRef { 2888, 552, 64, 38 };
const juce::Rectangle<int> kPresetNextRef { 2888, 596, 64, 38 };
const juce::Rectangle<int> kLeftFaderRef  { 120, 201, 81, 1530 };
const juce::Rectangle<int> kRightFaderRef { 3065, 204, 81, 1530 };
const juce::Rectangle<int> kLeftFaderCutoutRef  { 135, 217, 55, 1492 };
const juce::Rectangle<int> kRightFaderCutoutRef { 3080, 220, 55, 1492 };
const juce::Rectangle<int> kLeftFaderStraightPatchRef  { 70, 760, 200, 260 };
const juce::Rectangle<int> kRightFaderStraightPatchRef { 3056, 1790, 200, 260 };
const juce::Rectangle<int> kLeftFaderTopPatchRef  { 134, 201, 71, 160 };   // Clean top rail strip to reuse at the bottom.
const juce::Rectangle<int> kRightFaderTopPatchRef { 3161, 204, 71, 160 };  // Clean top rail strip to reuse at the bottom.
const juce::Rectangle<int> kLeftFaderBottomPatchRef  { 134, 1662, 71, 118 };   // Lower redraw area for the left big slider.
const juce::Rectangle<int> kRightFaderBottomPatchRef { 3161, 1662, 71, 118 };  // Lower redraw area for the right big slider.
const juce::Rectangle<int> kInputFaderStemCleanupRef       { 151, 1514,  30, 142 };
const juce::Rectangle<int> kOutputFaderStemCleanupRef      { 3080, 1514, 30, 142 };
const juce::Rectangle<int> kInputFaderStemCleanupSampleRef { 138, 1324,  56, 158 };
const juce::Rectangle<int> kOutputFaderStemCleanupSampleRef{ 3067, 1324, 56, 158 };
const juce::Rectangle<int> kInputFaderCapCleanupRef        { 116, 1608, 100,  76 };
const juce::Rectangle<int> kOutputFaderCapCleanupRef       { 3045, 1608, 100,  76 };
const juce::Rectangle<int> kInputFaderCapCleanupSampleRef  { 106, 1508, 120,  86 };
const juce::Rectangle<int> kOutputFaderCapCleanupSampleRef { 3035, 1508, 120,  86 };
const juce::Rectangle<int> kLeftVoiceAreaCleanupRef        { 52, 1638, 150, 290 };
const juce::Rectangle<int> kLeftFaderBottomCleanupRef      { 36, 1528, 270, 450 };
const juce::Rectangle<int> kRightFaderBottomCleanupRef { 3032, 1526, 820, 360 };
const juce::Rectangle<int> kLeftFaderBelowCleanupRef       { 96, 1742, 128, 44 };
const juce::Rectangle<int> kRightFaderBelowCleanupRef      { 3031, 1742, 128, 44 };
const juce::Rectangle<int> kRightFaderSideCleanupRef       { 3010, 1510, 0, 0 };
const juce::Rectangle<int> kRightFaderShadowCleanupRef     { 2960, 1600, 250, 170 };
const juce::Rectangle<int> kLeftFaderBottomCleanupSampleRef { 88, 1168, 150, 208 };
const juce::Rectangle<int> kRightFaderBottomCleanupSampleRef { 3017, 1168, 150, 208 };
const juce::Rectangle<int> kWidthThumbCleanupRef           { 2246, 1538, 128, 92 };
const juce::Rectangle<int> kWidthThumbCleanupSampleRef     { 2246, 1460, 128, 92 };
const juce::Rectangle<int> kWidthTrackRef                  { 2310, 1578, 232,  70 };

const std::array<juce::Rectangle<int>, 3> kDistortionCleanupRefs {{
    { 1869,  572, 78, 84 },
    { 1869, 1060, 78, 84 },
    { 1872, 1548, 78, 84 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionCleanupSampleRefs {{
    { 1852,  436, 112, 118 },
    { 1852,  924, 112, 118 },
    { 1855, 1412, 112, 118 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionTravelRefs {{
    { 1896,  343, 48, 258 },
    { 1896,  831, 48, 258 },
    { 1896, 1319, 48, 258 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionFaderRefs {{
    { 1832,  341, 66, 300 },
    { 1828,  828, 66, 300 },
    { 1830, 1317, 66, 300 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionCutoutRefs {{
    { 1852,  358, 29, 269 },
    { 1846,  846, 29, 269 },
    { 1846, 1334, 29, 269 }
}};

const juce::Rectangle<int> kWidthInnerTrackRef { 2271, 1579, 148, 30 };

// Static knob coverage areas — regions in bg_standard.png that contain baked-in knobs to paint over
// Tightly sized to only cover the knob + shadow footprint, staying within chassis boundaries
const juce::Rectangle<int> kLeftKnobCoverRef   { 92, 1588, 146, 168 };
const juce::Rectangle<int> kRightKnobCoverRef  { 3125, 1597, 146, 168 };
const std::array<juce::Rectangle<int>, 3> kDistKnobCoverRefs {{
    { 1838,  548, 118, 118 },
    { 1834, 1036, 118, 118 },
    { 1836, 1524, 118, 118 }
}};

const juce::Rectangle<int> kWidthKnobCoverRef { 2240, 1536, 150, 100 };

// Width slider track refs
const juce::Rectangle<int> kWidthFaderRef      { 2262, 1566, 340, 76 };
const juce::Rectangle<int> kWidthCutoutRef     { 2276, 1581, 318, 44 };

// Editable slider tuning values
const std::array<RectAdjust, 3> kDistortionSliderBoundsAdjust {{
    { -22, -12, 78, 24 },
    { -22, -12, 78, 24 },
    { -22, -12, 78, 24 }
}};

const std::array<PointAdjust, 3> kDistortionThumbOffset {{
    { -3.0f, 0.0f },
    { -2.0f, 0.0f },
    { -2.0f, 0.0f }
}};

const RectAdjust kWidthSliderBoundsAdjust { -14, 8, 34, 12 };
const PointAdjust kWidthThumbOffset { 0.0f, 2.0f };

// Visual track widths/heights from bg_standard.png (design px)
// These drive thumb sizing, independent of the wider hit-area components.
constexpr float kSideFaderKnobOuterSize = 108.895f;
constexpr float kSideFaderKnobTopSize = 77.782f;
constexpr float kSmallFaderKnobOuterSize = 56.0f;
constexpr float kSmallFaderKnobTopSize = 40.0f;

juce::String sanitisePresetDisplayName(juce::String name)
{
    name = name.trim();
    while (name.contains("  "))
        name = name.replace("  ", " ");
    name = name.replace("_", " ");
    return name.trim();
}

static void fillChassisBlendPatch(juce::Graphics& g,
                                  juce::Rectangle<float> area,
                                  float designScreenTop,
                                  float designScreenBot)
{
    // Keep vertical chassis relation like the rest of the panel
    const juce::Colour chassisTop (0xFFC8C9CB);
    const juce::Colour chassisBot (0xFFA6A7A9);

    // Base gradient, same family as the main chassis
    juce::ColourGradient bodyGrad(chassisTop,
                                  area.getCentreX(), designScreenTop,
                                  chassisBot,
                                  area.getCentreX(), designScreenBot,
                                  false);
    bodyGrad.addColour(0.52, juce::Colour(0xFFB8B9BB));
    g.setGradientFill(bodyGrad);
    g.fillEllipse(area);

    // Darker left-side merge so it visually joins the incoming slot shadow
    juce::ColourGradient leftMerge(juce::Colours::black.withAlpha(0.12f),
                                   area.getX() + area.getWidth() * 0.10f, area.getCentreY(),
                                   juce::Colours::transparentBlack,
                                   area.getX() + area.getWidth() * 0.42f, area.getCentreY(),
                                   false);
    g.setGradientFill(leftMerge);
    g.fillEllipse(area);

    // Mild right-side lift only, not too bright
    juce::ColourGradient rightLift(juce::Colours::transparentWhite,
                                   area.getX() + area.getWidth() * 0.56f, area.getCentreY(),
                                   juce::Colours::white.withAlpha(0.028f),
                                   area.getRight() - area.getWidth() * 0.10f, area.getCentreY(),
                                   false);
    g.setGradientFill(rightLift);
    g.fillEllipse(area);

    // Very soft top sheen
    juce::ColourGradient topGlow(juce::Colours::white.withAlpha(0.020f),
                                 area.getX() + area.getWidth() * 0.32f,
                                 area.getY() + area.getHeight() * 0.18f,
                                 juce::Colours::transparentWhite,
                                 area.getX() + area.getWidth() * 0.56f,
                                 area.getY() + area.getHeight() * 0.42f,
                                 true);
    g.setGradientFill(topGlow);
    g.fillEllipse(area.reduced(area.getWidth() * 0.08f, area.getHeight() * 0.08f));

    // Tiny lower-right falloff so it does not look flat
    juce::ColourGradient bottomFalloff(juce::Colours::transparentBlack,
                                       area.getX() + area.getWidth() * 0.52f,
                                       area.getY() + area.getHeight() * 0.46f,
                                       juce::Colours::black.withAlpha(0.045f),
                                       area.getX() + area.getWidth() * 0.84f,
                                       area.getY() + area.getHeight() * 0.88f,
                                       true);
    g.setGradientFill(bottomFalloff);
    g.fillEllipse(area);

    // Practically invisible edge
    g.setColour(juce::Colours::white.withAlpha(0.006f));
    g.drawEllipse(area.reduced(1.0f), 0.45f);
}

juce::String stripPresetOrderingPrefix(juce::String name)
{
    name = sanitisePresetDisplayName(name);

    int index = 0;
    while (index < name.length() && juce::CharacterFunctions::isDigit(name[index]))
        ++index;

    if (index > 0)
    {
        while (index < name.length() && (name[index] == ' ' || name[index] == '-' || name[index] == '_'))
            ++index;
        name = name.substring(index).trim();
    }

    return sanitisePresetDisplayName(name);
}

juce::String normaliseCategoryFolderName(juce::String folderName)
{
    folderName = sanitisePresetDisplayName(folderName);
    if (folderName.equalsIgnoreCase("Keys Synth")
        || folderName.equalsIgnoreCase("Keys / Synth")
        || folderName.equalsIgnoreCase("Keys _ Synth"))
        return "Keys / Synth";
    return folderName;
}

juce::String makePresetKey(const juce::String& category, const juce::String& preset)
{
    auto key = (normaliseCategoryFolderName(category) + "|" + sanitisePresetDisplayName(preset)).toLowerCase();
    key = key.removeCharacters(" .,_'!-+()/\\");
    return key;
}

juce::File findPresetImageRoot()
{
    juce::Array<juce::File> roots;
    roots.add(juce::File::getCurrentWorkingDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory());

    for (const auto& root : roots)
    {
        auto dir = root;
        for (int d = 0; d < 6; ++d)
        {
            const auto candidate = dir.getChildFile("Unison Mod PRESETS");
            if (candidate.isDirectory())
                return candidate;
            if (dir.isRoot())
                break;
            dir = dir.getParentDirectory();
        }
    }

    return {};
}

// LookAndFeel that draws NOTHING for linear sliders —
// all visual rendering is done in the editor's own paint() via drawSideFaderHandles etc.
struct NoThumbLAF : public juce::LookAndFeel_V4
{
    NoThumbLAF()
    {
        setColour(juce::Slider::thumbColourId,             juce::Colours::transparentBlack);
        setColour(juce::Slider::trackColourId,             juce::Colours::transparentBlack);
        setColour(juce::Slider::backgroundColourId,        juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderFillColourId,  juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override
    {
        // Explicitly clear — ensures nothing from parent path is visible
        g.setColour(juce::Colours::transparentBlack);
        g.fillRect(x, y, w, h);
    }

    void drawLinearSliderBackground (juce::Graphics&, int, int, int, int,
                                     float, float, float,
                                     juce::Slider::SliderStyle, juce::Slider&) override {}

    void drawLinearSliderThumb (juce::Graphics&, int, int, int, int,
                                float, float, float,
                                juce::Slider::SliderStyle, juce::Slider&) override {}

    int getSliderThumbRadius (juce::Slider&) override { return 0; }
};
static NoThumbLAF gNoThumbLAF;

} // namespace

// ============================================================================
ThreeVoicesAudioProcessorEditor::ThreeVoicesAudioProcessorEditor(ThreeVoicesAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    DBG("Working directory: " + juce::File::getCurrentWorkingDirectory().getFullPathName());
    DBG("Exe path: " + juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName());
    setOpaque(true);
    setResizable(false, false);

    loadImages();
    initialiseControls();
    initialiseAttachments();

    // SliderAttachment internally calls setVisible(true) — force them back invisible
    // so JUCE renders zero pixels for these sliders. Thumbs are drawn by paintOverChildren.
    for (auto& row : rows)
        row.distortionSlider.setVisible(false);
    widthSlider.setVisible(false);
    inputGainSlider.setVisible(false);
    outputGainSlider.setVisible(false);

    initialiseScreenAnimation();
    buildPresetPreviewLibrary();

    addAndMakeVisible(presetButton);
    addAndMakeVisible(previousPresetButton);
    addAndMakeVisible(nextPresetButton);
    presetButton.setLookAndFeel(&invisibleLookAndFeel);
    previousPresetButton.setLookAndFeel(&invisibleLookAndFeel);
    nextPresetButton.setLookAndFeel(&invisibleLookAndFeel);
    presetButton.onClick = [this] { openPresetOverlay(); };
    previousPresetButton.onClick = [this] { stepPreset(-1); };
    nextPresetButton.onClick = [this] { stepPreset(1); };

    for (const auto* id : kVoiceIds)      audioProcessor.getAPVTS().addParameterListener(id, this);
    for (const auto* id : kBitIds)        audioProcessor.getAPVTS().addParameterListener(id, this);
    for (const auto* id : kTubeIds)       audioProcessor.getAPVTS().addParameterListener(id, this);
    audioProcessor.getAPVTS().addParameterListener("presetChoice", this);
    audioProcessor.getAPVTS().addParameterListener("inputGain",    this);
    audioProcessor.getAPVTS().addParameterListener("outputGain",   this);
    audioProcessor.getAPVTS().addParameterListener("width",        this);
    audioProcessor.getAPVTS().addParameterListener("mix",          this);
    for (const auto* id : kSpeedIds)      audioProcessor.getAPVTS().addParameterListener(id, this);
    for (const auto* id : kDelayIds)      audioProcessor.getAPVTS().addParameterListener(id, this);
    for (const auto* id : kDepthIds)      audioProcessor.getAPVTS().addParameterListener(id, this);
    for (const auto* id : kDistortionIds) audioProcessor.getAPVTS().addParameterListener(id, this);

    cachedPresetName = getCurrentPresetName();
    setSize(1320, 760);
    startTimerHz(60);
}

ThreeVoicesAudioProcessorEditor::~ThreeVoicesAudioProcessorEditor()
{
    stopTimer();

    for (const auto* id : kVoiceIds)      audioProcessor.getAPVTS().removeParameterListener(id, this);
    for (const auto* id : kBitIds)        audioProcessor.getAPVTS().removeParameterListener(id, this);
    for (const auto* id : kTubeIds)       audioProcessor.getAPVTS().removeParameterListener(id, this);
    audioProcessor.getAPVTS().removeParameterListener("presetChoice", this);
    audioProcessor.getAPVTS().removeParameterListener("inputGain",    this);
    audioProcessor.getAPVTS().removeParameterListener("outputGain",   this);
    audioProcessor.getAPVTS().removeParameterListener("width",        this);
    audioProcessor.getAPVTS().removeParameterListener("mix",          this);
    for (const auto* id : kSpeedIds)      audioProcessor.getAPVTS().removeParameterListener(id, this);
    for (const auto* id : kDelayIds)      audioProcessor.getAPVTS().removeParameterListener(id, this);
    for (const auto* id : kDepthIds)      audioProcessor.getAPVTS().removeParameterListener(id, this);
    for (const auto* id : kDistortionIds) audioProcessor.getAPVTS().removeParameterListener(id, this);

    presetButton.setLookAndFeel(nullptr);
    previousPresetButton.setLookAndFeel(nullptr);
    nextPresetButton.setLookAndFeel(nullptr);
    widthSlider.setLookAndFeel(nullptr);
    mixKnob.setLookAndFeel(nullptr);
    inputGainSlider.setLookAndFeel(nullptr);
    outputGainSlider.setLookAndFeel(nullptr);

    for (auto& row : rows)
    {
        row.speedKnob.setLookAndFeel(nullptr);
        row.delayKnob.setLookAndFeel(nullptr);
        row.depthKnob.setLookAndFeel(nullptr);
        row.distortionSlider.setLookAndFeel(nullptr);
        row.voiceButton.setLookAndFeel(nullptr);
        row.bitButton.setLookAndFeel(nullptr);
        row.tubeButton.setLookAndFeel(nullptr);
    }
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::initialiseControls()
{
    auto setupRotary = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&invisibleLookAndFeel);
        s.setInterceptsMouseClicks(true, false);
        s.onValueChange = [this] { repaint(); };
        addAndMakeVisible(s);
    };
    auto setupVertical = [this](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&gNoThumbLAF);
        // addChildComponent = added to hierarchy (so getBounds() works for hit-testing
        // and APVTS attachment works) but NOT visible — zero pixels rendered by JUCE.
        // Mouse events are forwarded from the parent editor's mouseDown/Drag/Up.
        addChildComponent(s);
    };
    auto setupToggle = [this](juce::ToggleButton& b)
    {
        b.setLookAndFeel(&invisibleLookAndFeel);
        b.setClickingTogglesState(true);
        b.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(b);
    };

    for (auto& row : rows)
    {
        setupRotary(row.speedKnob);
        setupRotary(row.delayKnob);
        setupRotary(row.depthKnob);
        setupVertical(row.distortionSlider);
        setupToggle(row.voiceButton);
        setupToggle(row.bitButton);
        setupToggle(row.tubeButton);
    }

    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    widthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    widthSlider.setLookAndFeel(&gNoThumbLAF);
    addChildComponent(widthSlider);

    mixKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mixKnob.setLookAndFeel(&invisibleLookAndFeel);
    mixKnob.setInterceptsMouseClicks(true, false);
    mixKnob.onValueChange = [this] { repaint(); };
    addAndMakeVisible(mixKnob);

    inputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    inputGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    inputGainSlider.setMouseDragSensitivity(260);
    inputGainSlider.setLookAndFeel(&gNoThumbLAF);
    addChildComponent(inputGainSlider);

    outputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    outputGainSlider.setMouseDragSensitivity(260);
    outputGainSlider.setLookAndFeel(&gNoThumbLAF);
    addChildComponent(outputGainSlider);
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::initialiseAttachments()
{
    auto& apvts = audioProcessor.getAPVTS();
    for (int i = 0; i < 3; ++i)
    {
        auto& row = rows[i];
        row.speedAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, kSpeedIds[i],      row.speedKnob);
        row.delayAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, kDelayIds[i],      row.delayKnob);
        row.depthAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, kDepthIds[i],      row.depthKnob);
        row.distortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, kDistortionIds[i], row.distortionSlider);
        row.voiceAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, kVoiceIds[i],      row.voiceButton);
        row.bitAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, kBitIds[i],        row.bitButton);
        row.tubeAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, kTubeIds[i],       row.tubeButton);
    }
    widthAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "width",      widthSlider);
    mixAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix",        mixKnob);
    inputGainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "inputGain",  inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "outputGain", outputGainSlider);
}

void ThreeVoicesAudioProcessorEditor::initialiseSideFaderArt() {}

void ThreeVoicesAudioProcessorEditor::buildPresetPreviewLibrary()
{
    presetPreviewImages.clear();

    const auto& choices = audioProcessor.getFlattenedPresetChoices();
    for (int i = 0; i < choices.size(); ++i)
        presetPreviewImages.add({});

    const auto presetRoot = findPresetImageRoot();
    if (!presetRoot.isDirectory())
        return;

    juce::HashMap<juce::String, juce::File> imageFilesByKey;
    const auto categoryDirs = presetRoot.findChildFiles(juce::File::findDirectories, false);

    for (const auto& categoryDir : categoryDirs)
    {
        const auto category = normaliseCategoryFolderName(categoryDir.getFileName());
        const auto files = categoryDir.findChildFiles(juce::File::findFiles, false, "*");

        for (const auto& file : files)
        {
            const auto extension = file.getFileExtension().toLowerCase();
            if (extension != ".png" && extension != ".jpg" && extension != ".jpeg")
                continue;

            const auto presetName = stripPresetOrderingPrefix(file.getFileNameWithoutExtension());
            imageFilesByKey.set(makePresetKey(category, presetName), file);
        }
    }

    for (int i = 0; i < choices.size(); ++i)
    {
        const auto choice = choices[i];
        const auto separator = choice.indexOf(" - ");
        if (separator < 0)
            continue;

        const auto category = choice.substring(0, separator).trim();
        const auto preset = choice.substring(separator + 3).trim();
        const auto key = makePresetKey(category, preset);

        if (imageFilesByKey.contains(key))
        {
            auto image = juce::ImageCache::getFromFile(imageFilesByKey[key]);
            if (image.isValid())
                presetPreviewImages.set(i, image);
        }
    }
}

// ============================================================================
juce::File ThreeVoicesAudioProcessorEditor::findAssetFile(const juce::String& fileName) const
{
    juce::Array<juce::File> roots;
    roots.add(juce::File::getCurrentWorkingDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory());
    for (const auto& root : roots)
    {
        auto dir = root;
        for (int d = 0; d < 6; ++d)
        {
            if (dir.getChildFile(fileName).exists())                        return dir.getChildFile(fileName);
            if (dir.getChildFile("assets").getChildFile(fileName).exists()) return dir.getChildFile("assets").getChildFile(fileName);
            if (dir.getChildFile("Assets").getChildFile(fileName).exists()) return dir.getChildFile("Assets").getChildFile(fileName);
            if (dir.isRoot()) break;
            dir = dir.getParentDirectory();
        }
    }
    return {};
}

void ThreeVoicesAudioProcessorEditor::initialiseScreenAnimation()
{
    if (const auto animDir = findAssetFile("Assets").getChildFile("Anim"); animDir.isDirectory())
    {
        auto files = animDir.findChildFiles(juce::File::findFiles, false, "*.jpg");
        files.sort();
        for (const auto& file : files)
        {
            auto img = juce::ImageCache::getFromFile(file);
            if (img.isValid()) animationFrames.add(img);
        }
        if (!animationFrames.isEmpty()) { screenVideo.reset(); return; }
    }
}

void ThreeVoicesAudioProcessorEditor::loadImages()
{
    bgStandard  = loadImageByName("bg_standard.png");
    bgButtonsOn = loadImageByName("bg_buttons_on.png");
    menuCategories = loadImageByName("menu_categories.png");
    menuClassicMod = loadImageByName("menu_classic_modulation.png");
    menuGuitar     = loadImageByName("menu_guitar.png");
    menuKeysSynth  = loadImageByName("menu_keys_synth.png");
    menuBass       = loadImageByName("menu_bass.png");
    menuDrums      = loadImageByName("menu_drums.png");
    menuVocals     = loadImageByName("menu_vocals.png");
    sideFaderBottomCleanPatch = loadImageByName("clean_up.png");
}

juce::Image ThreeVoicesAudioProcessorEditor::loadImageByName(const juce::String& fileName) const
{
    auto fromBinary = [&]() -> juce::Image
    {
        const char* d = nullptr; int sz = 0;
        if      (fileName == "bg_standard.png")             { d = BinaryData::bg_standard_png;             sz = BinaryData::bg_standard_pngSize; }
        else if (fileName == "bg_buttons_on.png")           { d = BinaryData::bg_buttons_on_png;           sz = BinaryData::bg_buttons_on_pngSize; }
        else if (fileName == "menu_categories.png")         { d = BinaryData::menu_categories_png;         sz = BinaryData::menu_categories_pngSize; }
        else if (fileName == "menu_classic_modulation.png") { d = BinaryData::menu_classic_modulation_png; sz = BinaryData::menu_classic_modulation_pngSize; }
        else if (fileName == "menu_guitar.png")             { d = BinaryData::menu_guitar_png;             sz = BinaryData::menu_guitar_pngSize; }
        else if (fileName == "menu_keys_synth.png")         { d = BinaryData::menu_keys_synth_png;         sz = BinaryData::menu_keys_synth_pngSize; }
        else if (fileName == "menu_bass.png")               { d = BinaryData::menu_bass_png;               sz = BinaryData::menu_bass_pngSize; }
        else if (fileName == "menu_vocals.png")             { d = BinaryData::menu_vocals_png;             sz = BinaryData::menu_vocals_pngSize; }
        else if (fileName == "clean_up.png")                { d = BinaryData::clean_up_png;                sz = BinaryData::clean_up_pngSize; }
        if (d && sz > 0) return juce::ImageFileFormat::loadFrom(d, (size_t) sz);
        return {};
    };
    if (auto img = fromBinary(); img.isValid()) { DBG("Loaded " + fileName + " from binary"); return img; }
    DBG("Binary not valid for " + fileName + ", searching files");

    juce::Array<juce::File> roots;
    roots.add(juce::File::getCurrentWorkingDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory());
    roots.add(juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory());
    for (const auto& root : roots)
    {
        if (root.getChildFile(fileName).existsAsFile())                        return juce::ImageFileFormat::loadFrom(root.getChildFile(fileName));
        if (root.getChildFile("assets").getChildFile(fileName).existsAsFile()) return juce::ImageFileFormat::loadFrom(root.getChildFile("assets").getChildFile(fileName));
        if (root.getChildFile("Assets").getChildFile(fileName).existsAsFile()) return juce::ImageFileFormat::loadFrom(root.getChildFile("Assets").getChildFile(fileName));
    }
    return {};
}

// ============================================================================
juce::Rectangle<int> ThreeVoicesAudioProcessorEditor::scaleRect(const juce::Rectangle<int>& ref) const
{
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    const float ox = (getWidth()  - designW * us) * 0.5f;
    const float oy = (getHeight() - designH * us) * 0.5f;
    return { (int) std::round(ref.getX()     * us + ox), (int) std::round(ref.getY()     * us + oy),
             (int) std::round(ref.getWidth() * us),      (int) std::round(ref.getHeight()* us) };
}

bool ThreeVoicesAudioProcessorEditor::anyToggleActive() const
{
    auto& apvts = audioProcessor.getAPVTS();
    for (const auto* id : kVoiceIds) if (apvts.getRawParameterValue(id)->load() > 0.5f) return true;
    for (const auto* id : kBitIds)   if (apvts.getRawParameterValue(id)->load() > 0.5f) return true;
    for (const auto* id : kTubeIds)  if (apvts.getRawParameterValue(id)->load() > 0.5f) return true;
    return false;
}

juce::String ThreeVoicesAudioProcessorEditor::getCurrentPresetName() const
{
    const auto& choices = audioProcessor.getFlattenedPresetChoices();
    const int idx = juce::jlimit(0, juce::jmax(0, choices.size() - 1), audioProcessor.getCurrentPresetIndex());
    return choices.isEmpty() ? "PRESET SELECT" : choices[idx];
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (bgStandard.isValid())
    {
        g.drawImage(bgStandard,
                    0, 0, getWidth(), getHeight(),
                    0, 0, bgStandard.getWidth(), bgStandard.getHeight(),
                    false);
    }
    else { g.fillAll(juce::Colour(0xFFBFC0C2)); }

    drawSliderTrackBodies(g);
    drawSideFaderHandles(g);        // ← was missing
    drawDistortionSliderHandles(g); // ← was missing
    drawWidthSliderOverlay(g);      // ← was missing
    drawRotaryKnobOverlays(g);
    drawToggleOverlays(g);
    drawScreenFallbackAnimation(g);
    drawPresetDisplay(g);
}

void ThreeVoicesAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawToggleOverlays(juce::Graphics& g)
{
    if (!bgButtonsOn.isValid()) return;
    const float sx = bgButtonsOn.getWidth() / (float) designW;
    const float sy = bgButtonsOn.getHeight() / (float) designH;
    auto drawPatch = [&](const juce::Rectangle<int>& r)
    {
        const auto dst = scaleRect(r);
        g.drawImage(bgButtonsOn, dst.getX(), dst.getY(), dst.getWidth(), dst.getHeight(),
                    (int)std::round(r.getX()*sx),(int)std::round(r.getY()*sy),
                    (int)std::round(r.getWidth()*sx),(int)std::round(r.getHeight()*sy), false);
    };
    const std::array<juce::Rectangle<int>,3> voiceRefs {{ {307,291,283,378},{307,774,283,378},{307,1257,283,378} }};
    const std::array<juce::Rectangle<int>,3> bitRefs   {{ {1988,394,171,70},{1988,882,171,70},{1988,1370,171,70} }};
    const std::array<juce::Rectangle<int>,3> tubeRefs  {{ {1988,501,171,70},{1988,989,171,70},{1988,1477,171,70} }};
    for (int i = 0; i < 3; ++i)
    {
        if (rows[i].voiceButton.getToggleState()) drawPatch(voiceRefs[i]);
        if (rows[i].bitButton.getToggleState())   drawPatch(bitRefs[i]);
        if (rows[i].tubeButton.getToggleState())  drawPatch(tubeRefs[i]);
    }
}

void ThreeVoicesAudioProcessorEditor::drawScreenFallbackAnimation(juce::Graphics& g)
{
    const float us     = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    const auto  body   = scaleRect(kVideoRef).toFloat();
    const float corner = 8.0f * us;
    juce::Path clip;
    clip.addRoundedRectangle(body, corner);

    g.saveState();
    g.reduceClipRegion(clip);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(body, corner);

    if (!animationFrames.isEmpty())
    {
        g.drawImageWithin(animationFrames[currentAnimationFrame],
                          (int) body.getX(), (int) body.getY(),
                          (int) body.getWidth(), (int) body.getHeight(),
                          juce::RectanglePlacement::fillDestination, false);
    }

    g.restoreState();
}

void ThreeVoicesAudioProcessorEditor::drawPresetDisplay(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

// ============================================================================
// Shared silver sphere helper
// ============================================================================
// ============================================================================
// drawSilverCapsule — flat pill-shaped thumb matching bg_standard.png
// rect: the full bounding rectangle of the capsule (wider than tall for
//       horizontal; taller than wide for vertical distortion sliders)
// ============================================================================
static void drawSilverCapsule(juce::Graphics& g, const juce::Rectangle<float>& rect)
{
    const float r = juce::jmin(rect.getWidth(), rect.getHeight()) * 0.5f;  // corner radius

    g.setColour(juce::Colour(0xFF5F5F62));
    g.fillRoundedRectangle(rect, r);

    const auto inner = rect.reduced(rect.getWidth() * 0.07f, rect.getHeight() * 0.07f);
    {
        juce::ColourGradient grad(juce::Colour(0xFFE9E9EA),
                                  inner.getCentreX(), inner.getY(),
                                  juce::Colour(0xFFA5A5A8),
                                  inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(inner, juce::jmin(inner.getWidth(), inner.getHeight()) * 0.5f);
    }

    auto ridge = inner.reduced(inner.getWidth() * 0.12f, inner.getHeight() * 0.12f);
    {
        juce::ColourGradient ridgeGrad(juce::Colours::white.withAlpha(0.34f),
                                       ridge.getCentreX(), ridge.getY(),
                                       juce::Colour(0xFFD6D6D8).withAlpha(0.08f),
                                       ridge.getCentreX(), ridge.getBottom(), false);
        g.setGradientFill(ridgeGrad);
        g.fillRoundedRectangle(ridge.removeFromTop(ridge.getHeight() * 0.45f),
                               juce::jmin(ridge.getWidth(), ridge.getHeight()) * 0.4f);
    }

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.02f, rect.getHeight() * 0.02f),
                           r * 0.96f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.07f, rect.getHeight() * 0.07f),
                           r * 0.88f, 0.9f);
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.11f, rect.getHeight() * 0.11f),
                           r * 0.80f, 0.8f);
}

static void drawBlackSliderCover(juce::Graphics& g, const juce::Rectangle<float>& rect)
{
    const float r = juce::jmin(rect.getWidth(), rect.getHeight()) * 0.5f;
    g.setColour(juce::Colour(0xFF0A0B0D));
    g.fillRoundedRectangle(rect, r);

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.02f, rect.getHeight() * 0.02f),
                           r * 0.96f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.07f, rect.getHeight() * 0.07f),
                           r * 0.88f, 0.9f);
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    g.drawRoundedRectangle(rect.reduced(rect.getWidth() * 0.11f, rect.getHeight() * 0.11f),
                           r * 0.80f, 0.8f);
}

static void drawGreenFaderKnob(juce::Graphics& g, const juce::Rectangle<float>& outerRect)
{
    const float d = outerRect.getWidth();
    const auto shadow1 = outerRect.translated(d * 0.05f, d * 0.16f)
                                   .withSizeKeepingCentre(d * 0.92f, d * 0.42f);
    const auto shadow2 = outerRect.translated(d * 0.03f, d * 0.09f)
                                   .withSizeKeepingCentre(d * 0.76f, d * 0.30f);

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillEllipse(shadow1);
    g.setColour(juce::Colours::black.withAlpha(0.48f));
    g.fillEllipse(shadow2);

    {
        juce::ColourGradient rimGrad(juce::Colour(0xFF79D894), outerRect.getCentreX(), outerRect.getY(),
                                     juce::Colour(0xFF248E4D), outerRect.getCentreX(), outerRect.getBottom(), false);
        rimGrad.addColour(0.42, juce::Colour(0xFF49B96A));
        rimGrad.addColour(0.72, juce::Colour(0xFF309A56));
        g.setGradientFill(rimGrad);
        g.fillEllipse(outerRect);
    }

    const auto inner = outerRect.reduced(d * 0.10f);
    {
        juce::ColourGradient innerGrad(juce::Colour(0xFF61CF84), inner.getCentreX(), inner.getY(),
                                       juce::Colour(0xFF39A85D), inner.getCentreX(), inner.getBottom(), false);
        innerGrad.addColour(0.50, juce::Colour(0xFF47B86A));
        g.setGradientFill(innerGrad);
        g.fillEllipse(inner);
    }

    {
        auto sheen = inner.withHeight(inner.getHeight() * 0.48f).translated(0.0f, inner.getHeight() * 0.03f);
        juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(0.22f),
                                       sheen.getCentreX(), sheen.getY(),
                                       juce::Colours::transparentWhite,
                                       sheen.getCentreX(), sheen.getBottom(), false);
        g.setGradientFill(sheenGrad);
        g.fillEllipse(sheen);
    }

    {
        auto bottomShade = inner.withY(inner.getCentreY()).withHeight(inner.getHeight() * 0.42f);
        juce::ColourGradient shadeGrad(juce::Colours::transparentBlack,
                                       bottomShade.getCentreX(), bottomShade.getY(),
                                       juce::Colours::black.withAlpha(0.16f),
                                       bottomShade.getCentreX(), bottomShade.getBottom(), false);
        g.setGradientFill(shadeGrad);
        g.fillEllipse(bottomShade);
    }

    g.setColour(juce::Colours::white.withAlpha(0.26f));
    g.drawEllipse(inner.reduced(d * 0.02f), 1.0f);

    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.drawEllipse(outerRect.reduced(d * 0.01f), 1.1f);
}

static void drawSilverFaderKnob(juce::Graphics& g, const juce::Rectangle<float>& outerRect)
{
    const float d = outerRect.getWidth();
    const auto shadow1 = outerRect.translated(d * 0.04f, d * 0.13f)
                                   .withSizeKeepingCentre(d * 0.90f, d * 0.34f);
    const auto shadow2 = outerRect.translated(d * 0.02f, d * 0.08f)
                                   .withSizeKeepingCentre(d * 0.72f, d * 0.24f);

    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillEllipse(shadow1);
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillEllipse(shadow2);

    {
        juce::ColourGradient outerGrad(juce::Colour(0xFFF5F5F6), outerRect.getCentreX(), outerRect.getY(),
                                       juce::Colour(0xFFA9A9AD), outerRect.getCentreX(), outerRect.getBottom(), false);
        outerGrad.addColour(0.38, juce::Colour(0xFFD7D7DA));
        outerGrad.addColour(0.68, juce::Colour(0xFFBEBEC2));
        g.setGradientFill(outerGrad);
        g.fillEllipse(outerRect);
    }

    const auto inner = outerRect.reduced(d * 0.11f);

    {
        juce::ColourGradient innerGrad(juce::Colour(0xFFE7E7EA), inner.getCentreX(), inner.getY(),
                                       juce::Colour(0xFFC6C6CA), inner.getCentreX(), inner.getBottom(), false);
        innerGrad.addColour(0.55, juce::Colour(0xFFD6D6DA));
        g.setGradientFill(innerGrad);
        g.fillEllipse(inner);
    }

    {
        auto topLight = inner.withHeight(inner.getHeight() * 0.44f).translated(0.0f, inner.getHeight() * 0.02f);
        juce::ColourGradient topGrad(juce::Colours::white.withAlpha(0.30f),
                                     topLight.getCentreX(), topLight.getY(),
                                     juce::Colours::transparentWhite,
                                     topLight.getCentreX(), topLight.getBottom(), false);
        g.setGradientFill(topGrad);
        g.fillEllipse(topLight);
    }

    {
        auto bottomShade = inner.withY(inner.getCentreY()).withHeight(inner.getHeight() * 0.40f);
        juce::ColourGradient shadeGrad(juce::Colours::transparentBlack,
                                       bottomShade.getCentreX(), bottomShade.getY(),
                                       juce::Colours::black.withAlpha(0.14f),
                                       bottomShade.getCentreX(), bottomShade.getBottom(), false);
        g.setGradientFill(shadeGrad);
        g.fillEllipse(bottomShade);
    }

    g.setColour(juce::Colours::white.withAlpha(0.30f));
    g.drawEllipse(inner.reduced(d * 0.02f), 0.9f);

    g.setColour(juce::Colours::black.withAlpha(0.16f));
    g.drawEllipse(outerRect.reduced(d * 0.01f), 1.0f);
}

static void fillCleanupTexture(juce::Graphics& g, const juce::Image& image,
                               const juce::Rectangle<float>& area, float corner)
{
    if (! image.isValid())
    {
        g.setColour(juce::Colour(0xFFC7C8CA));
        g.fillRoundedRectangle(area, corner);
        return;
    }

    juce::Path clip;
    clip.addRoundedRectangle(area, corner);
    g.saveState();
    g.reduceClipRegion(clip);
    g.drawImageWithin(image,
                      (int) std::round(area.getX()), (int) std::round(area.getY()),
                      (int) std::round(area.getWidth()), (int) std::round(area.getHeight()),
                      juce::RectanglePlacement::stretchToFit, false);
    g.restoreState();
}

static void fillCleanupTextureEllipse(juce::Graphics& g, const juce::Image& image,
                                      const juce::Rectangle<float>& area)
{
    if (! image.isValid())
    {
        g.setColour(juce::Colour(0xFFC7C8CA));
        g.fillEllipse(area);
        return;
    }

    juce::Path clip;
    clip.addEllipse(area);
    g.saveState();
    g.reduceClipRegion(clip);
    g.drawImageWithin(image,
                      (int) std::round(area.getX()), (int) std::round(area.getY()),
                      (int) std::round(area.getWidth()), (int) std::round(area.getHeight()),
                      juce::RectanglePlacement::stretchToFit, false);
    g.restoreState();
}

static void drawPatchFromImage(juce::Graphics& g, const juce::Image& sourceImage,
                               const juce::Rectangle<int>& sourceRef, const juce::Rectangle<float>& destArea,
                               int designW, int designH)
{
    if (! sourceImage.isValid())
        return;

    const float sx = sourceImage.getWidth() / (float) designW;
    const float sy = sourceImage.getHeight() / (float) designH;
    const juce::Rectangle<int> srcPx {
        (int) std::round(sourceRef.getX() * sx),
        (int) std::round(sourceRef.getY() * sy),
        (int) std::round(sourceRef.getWidth() * sx),
        (int) std::round(sourceRef.getHeight() * sy)
    };

    auto patch = sourceImage.getClippedImage(srcPx);
    const float scaleX = destArea.getWidth() / (float) patch.getWidth();
    const float scaleY = destArea.getHeight() / (float) patch.getHeight();
    auto transform = juce::AffineTransform::scale(scaleX, scaleY)
                        .translated(destArea.getX(), destArea.getY());
    g.drawImageTransformed(patch, transform, false);
}

static void drawPatchFromImageWithBottomCurve(juce::Graphics& g, const juce::Image& sourceImage,
                                              const juce::Rectangle<int>& sourceRef, const juce::Rectangle<float>& destArea,
                                              int designW, int designH, float cornerRadius)
{
    if (! sourceImage.isValid())
        return;

    const float sx = sourceImage.getWidth() / (float) designW;
    const float sy = sourceImage.getHeight() / (float) designH;
    const juce::Rectangle<int> srcPx {
        (int) std::round(sourceRef.getX() * sx),
        (int) std::round(sourceRef.getY() * sy),
        (int) std::round(sourceRef.getWidth() * sx),
        (int) std::round(sourceRef.getHeight() * sy)
    };

    auto patch = sourceImage.getClippedImage(srcPx);
    const float scaleX = destArea.getWidth() / (float) patch.getWidth();
    const float scaleY = destArea.getHeight() / (float) patch.getHeight();
    auto transform = juce::AffineTransform::scale(scaleX, scaleY)
                        .translated(destArea.getX(), destArea.getY());

    juce::Path clip;
    const float r = juce::jmin(cornerRadius, destArea.getWidth() * 0.5f, destArea.getHeight() * 0.5f);
    clip.startNewSubPath(destArea.getX(), destArea.getY());
    clip.lineTo(destArea.getRight(), destArea.getY());
    clip.lineTo(destArea.getRight(), destArea.getBottom() - r);
    clip.addArc(destArea.getRight() - 2.0f * r, destArea.getBottom() - 2.0f * r,
                2.0f * r, 2.0f * r, 0.0f, juce::MathConstants<float>::halfPi, true);
    clip.lineTo(destArea.getX() + r, destArea.getBottom());
    clip.addArc(destArea.getX(), destArea.getBottom() - 2.0f * r,
                2.0f * r, 2.0f * r, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi, true);
    clip.closeSubPath();

    g.saveState();
    g.reduceClipRegion(clip);
    g.drawImageTransformed(patch, transform, false);
    g.restoreState();
}

static void drawPatchFromImageRounded(juce::Graphics& g, const juce::Image& sourceImage,
                                      const juce::Rectangle<int>& sourceRef, const juce::Rectangle<float>& destArea,
                                      int designW, int designH, float cornerRadius)
{
    if (! sourceImage.isValid())
        return;

    const float sx = sourceImage.getWidth() / (float) designW;
    const float sy = sourceImage.getHeight() / (float) designH;
    const juce::Rectangle<int> srcPx {
        (int) std::round(sourceRef.getX() * sx),
        (int) std::round(sourceRef.getY() * sy),
        (int) std::round(sourceRef.getWidth() * sx),
        (int) std::round(sourceRef.getHeight() * sy)
    };

    auto patch = sourceImage.getClippedImage(srcPx);
    const float scaleX = destArea.getWidth() / (float) patch.getWidth();
    const float scaleY = destArea.getHeight() / (float) patch.getHeight();
    auto transform = juce::AffineTransform::scale(scaleX, scaleY)
                        .translated(destArea.getX(), destArea.getY());

    juce::Path clip;
    clip.addRoundedRectangle(destArea, cornerRadius);
    g.saveState();
    g.reduceClipRegion(clip);
    g.drawImageTransformed(patch, transform, false);
    g.restoreState();
}

static void drawPatchFromImageEllipse(juce::Graphics& g, const juce::Image& sourceImage,
                                      const juce::Rectangle<int>& sourceRef, const juce::Rectangle<float>& destArea,
                                      int designW, int designH)
{
    if (! sourceImage.isValid())
        return;

    const float sx = sourceImage.getWidth() / (float) designW;
    const float sy = sourceImage.getHeight() / (float) designH;
    const juce::Rectangle<int> srcPx {
        (int) std::round(sourceRef.getX() * sx),
        (int) std::round(sourceRef.getY() * sy),
        (int) std::round(sourceRef.getWidth() * sx),
        (int) std::round(sourceRef.getHeight() * sy)
    };

    auto patch = sourceImage.getClippedImage(srcPx);
    const float scaleX = destArea.getWidth() / (float) patch.getWidth();
    const float scaleY = destArea.getHeight() / (float) patch.getHeight();
    auto transform = juce::AffineTransform::scale(scaleX, scaleY)
                        .translated(destArea.getX(), destArea.getY());

    juce::Path clip;
    clip.addEllipse(destArea);
    g.saveState();
    g.reduceClipRegion(clip);
    g.drawImageTransformed(patch, transform, false);
    g.restoreState();
}

// ============================================================================
// SLIDER TRACK BODIES — covers static knobs from bg, draws CSS tracks
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawSliderTrackBodies(juce::Graphics& g)
{
    if (!bgStandard.isValid()) return;
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    const float designScreenTop = (getHeight() - designH * us) * 0.5f;
    const float designScreenBot = designScreenTop + designH * us;
    const juce::Colour chassisTop(0xFFC8C9CB);
    const juce::Colour chassisBot(0xFFA6A7A9);

    auto fillChassis = [&](juce::Rectangle<float> area)
    {
        juce::ColourGradient grad(chassisTop, area.getCentreX(), designScreenTop,
                                  chassisBot, area.getCentreX(), designScreenBot, false);
        g.setGradientFill(grad);
        g.fillRect(area);
    };

    auto fillChassisRect = fillChassis;

    auto fillChassisEllipse = [&](juce::Rectangle<float> area)
    {
        juce::ColourGradient grad(chassisTop, area.getCentreX(), designScreenTop,
                                  chassisBot, area.getCentreX(), designScreenBot, false);
        g.setGradientFill(grad);
        g.fillEllipse(area);

        g.setColour(juce::Colours::white.withAlpha(0.035f));
        g.drawEllipse(area.reduced(0.5f), 0.8f);

        g.setColour(juce::Colours::black.withAlpha(0.02f));
        g.drawEllipse(area.reduced(1.0f), 0.8f);
    };


    {
        auto leftVoiceCleanup = scaleRect(kLeftVoiceAreaCleanupRef).toFloat();
        juce::ColourGradient cleanupGrad(chassisTop, leftVoiceCleanup.getCentreX(), designScreenTop,
                                         chassisBot, leftVoiceCleanup.getCentreX(), designScreenBot, false);
        g.setGradientFill(cleanupGrad);
        g.fillRect(leftVoiceCleanup);
    }


        auto fillChassisRoundedPatch = [&](juce::Rectangle<float> area, float corner)
    {
        juce::ColourGradient grad(chassisTop, area.getCentreX(), designScreenTop,
                                  chassisBot, area.getCentreX(), designScreenBot, false);
        grad.addColour(0.52, juce::Colour(0xFFB8B9BB));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(area, corner);

        juce::ColourGradient leftMerge(juce::Colours::black.withAlpha(0.08f),
                                       area.getX() + area.getWidth() * 0.12f, area.getCentreY(),
                                       juce::Colours::transparentBlack,
                                       area.getX() + area.getWidth() * 0.38f, area.getCentreY(),
                                       false);
        g.setGradientFill(leftMerge);
        g.fillRoundedRectangle(area, corner);

        juce::ColourGradient rightLift(juce::Colours::transparentWhite,
                                       area.getX() + area.getWidth() * 0.55f, area.getCentreY(),
                                       juce::Colours::white.withAlpha(0.025f),
                                       area.getRight() - area.getWidth() * 0.10f, area.getCentreY(),
                                       false);
        g.setGradientFill(rightLift);
        g.fillRoundedRectangle(area, corner);
    };

    // Helper: draw CSS vertical fader track, CLIPPED to coverRef
    auto drawVerticalTrackClipped = [&](const juce::Rectangle<int>& faderRef,
                                        const juce::Rectangle<int>& cutoutRef,
                                        const juce::Rectangle<int>& clipRef)
    {
        auto clipArea = scaleRect(clipRef);
        g.saveState();
        g.reduceClipRegion(clipArea);

        auto indent = scaleRect(faderRef).toFloat();
        auto cutout = scaleRect(cutoutRef).toFloat();
        float indentR = indent.getWidth() * 0.45f;
        float cutoutR = cutout.getWidth() * 0.64f;

        // Outer highlight (CSS: 3px 3px 2px rgba(255,255,255,0.8))
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillRoundedRectangle(indent.translated(2.0f * us, 2.0f * us), indentR);
        // Dark shadow (CSS: -1px -2px 2px 2px rgba(0,0,0,0.55))
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillRoundedRectangle(indent.translated(-1.0f * us, -1.5f * us), indentR);

        // Indent body
        juce::ColourGradient indGrad(juce::Colour(0xFFBBBBBD), indent.getCentreX(), indent.getY(),
                                     juce::Colour(0xFFB1B1B3), indent.getCentreX(), indent.getBottom(), false);
        g.setGradientFill(indGrad);
        g.fillRoundedRectangle(indent, indentR);

        // Border
        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.drawRoundedRectangle(indent, indentR, us);

        // Inset bottom shadow
        {
            auto shadowArea = indent.withTop(indent.getBottom() - 16.0f * us);
            juce::ColourGradient shGrad(juce::Colours::transparentBlack, shadowArea.getCentreX(), shadowArea.getY(),
                                        juce::Colours::black.withAlpha(0.50f), shadowArea.getCentreX(), shadowArea.getBottom(), false);
            g.setGradientFill(shGrad);
            juce::Path shPath;
            shPath.addRoundedRectangle(shadowArea, indentR);
            g.fillPath(shPath);
        }
        // Left highlight line
        g.setColour(juce::Colours::white.withAlpha(0.30f));
        g.drawLine(indent.getX() + 2.0f * us, indent.getY() + indentR,
                   indent.getX() + 2.0f * us, indent.getBottom() - indentR, us * 1.2f);

        // Cutout
        g.setColour(juce::Colour(0xFF0F0F11));
        g.fillRoundedRectangle(cutout, cutoutR);
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawRoundedRectangle(cutout, cutoutR, us * 0.8f);
        // Inset shadow
        g.setColour(juce::Colours::black.withAlpha(0.40f));
        g.drawLine(cutout.getX() + 2.0f * us, cutout.getY() + cutoutR,
                   cutout.getX() + 2.0f * us, cutout.getY() + cutout.getHeight() * 0.25f, us * 1.2f);

        g.restoreState();
    };

    auto drawVerticalTrackFull = [&](const juce::Rectangle<int>& faderRef,
                                     const juce::Rectangle<int>& cutoutRef,
                                     const juce::Rectangle<int>& cleanupRef,
                                     const juce::Rectangle<int>& cleanupSampleRef,
                                     bool drawCleanupPatch,
                                     float bodyOffsetX,
                                     float innerLaneOffsetX,
                                     float exteriorShadowBottomTrim,
                                     float darkBlurAlpha,
                                     float lightBlurAlpha,
                                     float outerHighlightAlpha,
                                     float outerShadowAlpha,
                                     float lowerShadowAlpha,
                                     float cutoutLeftShadeAlpha,
                                     float cutoutRightGlowAlpha,
                                     float coreLeftAlpha,
                                     float coreRightAlpha)
    {
        juce::ignoreUnused(cleanupSampleRef);
        if (drawCleanupPatch)
        {
            auto cleanupArea = scaleRect(cleanupRef).toFloat();
            fillCleanupTexture(g, sideFaderBottomCleanPatch, cleanupArea, cleanupArea.getWidth() * 0.46f);
        }

        auto indent = scaleRect(faderRef).toFloat().translated(bodyOffsetX * us, 0.0f);
        auto cutout = scaleRect(cutoutRef).toFloat().translated(bodyOffsetX * us, 0.0f);
        const float indentR = 64.0f * us;
        const float cutoutR = 32.0f * us;

        // CSS shadow rectangles 60 / 61
        auto darkBlurRect = indent.withPosition(indent.getX() - 6.0f * us, indent.getY() - 23.0f * us)
                                  .withSizeKeepingCentre(83.0f * us, 1606.0f * us);
        auto lightBlurRect = indent.withPosition(indent.getX() - 10.0f * us, indent.getY() - 17.0f * us)
                                   .withSizeKeepingCentre(108.0f * us, 1739.0f * us);
        auto shadowClip = indent.expanded(18.0f * us, 18.0f * us);
        if (exteriorShadowBottomTrim > 0.0f)
            shadowClip.setBottom(shadowClip.getBottom() - exteriorShadowBottomTrim * us);

        g.saveState();
        g.reduceClipRegion(shadowClip.getSmallestIntegerContainer());
        g.setColour(juce::Colour(0xFF636363).withAlpha(darkBlurAlpha));
        g.fillRoundedRectangle(darkBlurRect, 64.0f * us);
        g.setColour(juce::Colour(0xFFD9D8DB).withAlpha(lightBlurAlpha));
        g.fillRoundedRectangle(lightBlurRect, 70.0f * us);
        g.restoreState();

        // Main indent shadows from CSS
        g.setColour(juce::Colours::white.withAlpha(outerHighlightAlpha));
        g.fillRoundedRectangle(indent.translated(3.0f * us, 3.0f * us), indentR);
        g.setColour(juce::Colours::black.withAlpha(outerShadowAlpha));
        g.fillRoundedRectangle(indent.translated(-1.0f * us, -2.0f * us), indentR);

        juce::ColourGradient indGrad(juce::Colour(0xFFDADBDD), indent.getCentreX(), indent.getY(),
                                     juce::Colour(0xFFC8C9CB), indent.getCentreX(), indent.getBottom(), false);
        g.setGradientFill(indGrad);
        g.fillRoundedRectangle(indent, indentR);

        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.drawRoundedRectangle(indent, indentR, us);

        // inset -8px 4px bottom shadow
        auto lowerShadowArea = indent.withTop(indent.getBottom() - 28.0f * us);
        juce::ColourGradient lowerShadow(juce::Colours::transparentBlack, lowerShadowArea.getCentreX(), lowerShadowArea.getY(),
                                         juce::Colours::black.withAlpha(lowerShadowAlpha), lowerShadowArea.getCentreX(), lowerShadowArea.getBottom(), false);
        g.setGradientFill(lowerShadow);
        g.fillRoundedRectangle(lowerShadowArea, indentR);

        // inset -3 -3 2 white highlight
        g.setColour(juce::Colours::white.withAlpha(0.26f));
        g.drawLine(indent.getX() + 2.0f * us, indent.getY() + indentR,
                   indent.getX() + 2.0f * us, indent.getBottom() - indentR, us * 1.0f);

        g.setColour(juce::Colour(0xFF0F0F11));
        g.fillRoundedRectangle(cutout, cutoutR);
        g.setColour(juce::Colours::white.withAlpha(0.51f));
        g.drawRoundedRectangle(cutout, cutoutR, us * 0.9f);

        auto cutoutLeftShade = juce::Rectangle<float>(2.0f * us, cutout.getHeight() - 2.0f * us)
                                   .withPosition(cutout.getX() + 2.0f * us, cutout.getY() + 1.0f * us);
        auto cutoutRightGlow = juce::Rectangle<float>(2.0f * us, cutout.getHeight() - 2.0f * us)
                                   .withPosition(cutout.getRight() - 4.0f * us, cutout.getY() + 1.0f * us);
        g.setColour(juce::Colours::black.withAlpha(cutoutLeftShadeAlpha));
        g.fillRoundedRectangle(cutoutLeftShade, cutoutLeftShade.getWidth() * 0.5f);
        g.setColour(juce::Colours::white.withAlpha(cutoutRightGlowAlpha));
        g.fillRoundedRectangle(cutoutRightGlow, cutoutRightGlow.getWidth() * 0.5f);

        // Center the darker inner lane and keep its ends rounded so the top/bottom
        // stay smooth instead of exposing a hard rectangular edge.
        auto centreShadow = juce::Rectangle<float>(12.0f * us, cutout.getHeight() - 4.0f * us)
                                .withCentre(cutout.getCentre())
                                .translated(innerLaneOffsetX * us, 0.0f);
        const float centreShadowR = centreShadow.getWidth() * 0.5f;

        g.setColour(juce::Colour(0xFF0B0C0E));
        g.fillRoundedRectangle(centreShadow, centreShadowR);

        auto leftCore = juce::Rectangle<float>(4.0f * us, centreShadow.getHeight())
                            .withPosition(centreShadow.getX(), centreShadow.getY());
        auto rightRim = juce::Rectangle<float>(4.0f * us, centreShadow.getHeight())
                            .withPosition(centreShadow.getRight() - 4.0f * us, centreShadow.getY());
        g.setColour(juce::Colours::black.withAlpha(coreLeftAlpha));
        g.fillRoundedRectangle(leftCore, leftCore.getWidth() * 0.5f);
        g.setColour(juce::Colours::white.withAlpha(coreRightAlpha));
        g.fillRoundedRectangle(rightRim, rightRim.getWidth() * 0.5f);

    };

    auto drawVerticalTrackFullSmall = [&](const juce::Rectangle<int>& faderRef,
                                         const juce::Rectangle<int>& cutoutRef,
                                         const juce::Rectangle<int>& cleanupRef,
                                         bool drawCleanupPatch,
                                         float bodyOffsetX,
                                         float innerLaneOffsetX,
                                         float darkBlurAlpha,
                                         float lightBlurAlpha,
                                         float outerHighlightAlpha,
                                         float outerShadowAlpha,
                                         float lowerShadowAlpha,
                                         float cutoutLeftShadeAlpha,
                                         float cutoutRightGlowAlpha,
                                         float coreLeftAlpha,
                                         float coreRightAlpha)
    {
        if (drawCleanupPatch)
        {
            auto cleanupArea = scaleRect(cleanupRef).toFloat();

            juce::ColourGradient cleanupGrad(chassisTop, cleanupArea.getCentreX(), designScreenTop,
                                             chassisBot, cleanupArea.getCentreX(), designScreenBot, false);
            g.setGradientFill(cleanupGrad);
            g.fillEllipse(cleanupArea);
        }

        auto indent = scaleRect(faderRef).toFloat().translated(bodyOffsetX * us, 0.0f);
        auto cutout = scaleRect(cutoutRef).toFloat().translated(bodyOffsetX * us, 0.0f);

        const float indentR = indent.getWidth() * 0.46f;
        const float cutoutR = cutout.getWidth() * 0.62f;

        auto darkBlurRect = indent.expanded(6.0f * us, 10.0f * us).translated(-2.0f * us, -3.0f * us);
        auto lightBlurRect = indent.expanded(10.0f * us, 14.0f * us).translated(-3.0f * us, -2.0f * us);

        g.saveState();
        g.reduceClipRegion(indent.expanded(18.0f * us, 18.0f * us).getSmallestIntegerContainer());
        g.setColour(juce::Colour(0xFF636363).withAlpha(darkBlurAlpha));
        g.fillRoundedRectangle(darkBlurRect, darkBlurRect.getWidth() * 0.46f);
        g.setColour(juce::Colour(0xFFD9D8DB).withAlpha(lightBlurAlpha));
        g.fillRoundedRectangle(lightBlurRect, lightBlurRect.getWidth() * 0.48f);
        g.restoreState();

        g.setColour(juce::Colours::white.withAlpha(outerHighlightAlpha));
        g.fillRoundedRectangle(indent.translated(2.0f * us, 2.0f * us), indentR);

        g.setColour(juce::Colours::black.withAlpha(outerShadowAlpha));
        g.fillRoundedRectangle(indent.translated(-1.0f * us, -1.5f * us), indentR);

        juce::ColourGradient indGrad(juce::Colour(0xFFDADBDD), indent.getCentreX(), indent.getY(),
                                     juce::Colour(0xFFC8C9CB), indent.getCentreX(), indent.getBottom(), false);
        g.setGradientFill(indGrad);
        g.fillRoundedRectangle(indent, indentR);

        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.drawRoundedRectangle(indent, indentR, us * 0.8f);

        auto lowerShadowArea = indent.withTop(indent.getBottom() - 18.0f * us);
        juce::ColourGradient lowerShadow(juce::Colours::transparentBlack,
                                         lowerShadowArea.getCentreX(), lowerShadowArea.getY(),
                                         juce::Colours::black.withAlpha(lowerShadowAlpha),
                                         lowerShadowArea.getCentreX(), lowerShadowArea.getBottom(), false);
        g.setGradientFill(lowerShadow);
        g.fillRoundedRectangle(lowerShadowArea, indentR);

        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawLine(indent.getX() + 1.5f * us, indent.getY() + indentR,
                   indent.getX() + 1.5f * us, indent.getBottom() - indentR, us * 0.9f);

        g.setColour(juce::Colour(0xFF0F0F11));
        g.fillRoundedRectangle(cutout, cutoutR);

        g.setColour(juce::Colours::white.withAlpha(0.42f));
        g.drawRoundedRectangle(cutout, cutoutR, us * 0.75f);

        auto cutoutLeftShade = juce::Rectangle<float>(1.5f * us, cutout.getHeight() - 2.0f * us)
                                   .withPosition(cutout.getX() + 1.5f * us, cutout.getY() + 1.0f * us);
        auto cutoutRightGlow = juce::Rectangle<float>(1.5f * us, cutout.getHeight() - 2.0f * us)
                                   .withPosition(cutout.getRight() - 3.0f * us, cutout.getY() + 1.0f * us);

        g.setColour(juce::Colours::black.withAlpha(cutoutLeftShadeAlpha));
        g.fillRoundedRectangle(cutoutLeftShade, cutoutLeftShade.getWidth() * 0.5f);

        g.setColour(juce::Colours::white.withAlpha(cutoutRightGlowAlpha));
        g.fillRoundedRectangle(cutoutRightGlow, cutoutRightGlow.getWidth() * 0.5f);

        auto centreShadow = juce::Rectangle<float>(8.0f * us, cutout.getHeight() - 4.0f * us)
                                .withCentre(cutout.getCentre())
                                .translated(innerLaneOffsetX * us, 0.0f);
        const float centreShadowR = centreShadow.getWidth() * 0.5f;

        g.setColour(juce::Colour(0xFF0B0C0E));
        g.fillRoundedRectangle(centreShadow, centreShadowR);

        auto leftCore = juce::Rectangle<float>(2.5f * us, centreShadow.getHeight())
                            .withPosition(centreShadow.getX(), centreShadow.getY());
        auto rightRim = juce::Rectangle<float>(2.5f * us, centreShadow.getHeight())
                            .withPosition(centreShadow.getRight() - 2.5f * us, centreShadow.getY());

        g.setColour(juce::Colours::black.withAlpha(coreLeftAlpha));
        g.fillRoundedRectangle(leftCore, leftCore.getWidth() * 0.5f);

        g.setColour(juce::Colours::white.withAlpha(coreRightAlpha));
        g.fillRoundedRectangle(rightRim, rightRim.getWidth() * 0.5f);
    };

    auto drawHorizontalTrackFull = [&](const juce::Rectangle<int>& faderRef,
                                       const juce::Rectangle<int>& cutoutRef,
                                       const juce::Rectangle<int>& cleanupRef,
                                       bool drawCleanupPatch,
                                       float bodyOffsetY,
                                       float innerLaneOffsetY,
                                       float darkBlurAlpha,
                                       float lightBlurAlpha,
                                       float outerHighlightAlpha,
                                       float outerShadowAlpha,
                                       float cutoutTopShadeAlpha,
                                       float cutoutBottomGlowAlpha,
                                       float coreTopAlpha,
                                       float coreBottomAlpha)
    {
        if (drawCleanupPatch)
        {
            auto cleanupArea = scaleRect(cleanupRef).toFloat();
            juce::ColourGradient cleanupGrad(chassisTop, cleanupArea.getCentreX(), designScreenTop,
                                             chassisBot, cleanupArea.getCentreX(), designScreenBot, false);
            g.setGradientFill(cleanupGrad);
            g.fillEllipse(cleanupArea);
        }

        auto indent = scaleRect(faderRef).toFloat().translated(0.0f, bodyOffsetY * us);
        auto cutout = scaleRect(cutoutRef).toFloat().translated(0.0f, bodyOffsetY * us);

        const float indentR = indent.getHeight() * 0.46f;
        const float cutoutR = cutout.getHeight() * 0.62f;

        auto darkBlurRect = indent.expanded(10.0f * us, 6.0f * us).translated(-2.0f * us, -2.0f * us);
        auto lightBlurRect = indent.expanded(14.0f * us, 9.0f * us).translated(-1.0f * us, -2.0f * us);

        g.saveState();
        g.reduceClipRegion(indent.expanded(18.0f * us, 18.0f * us).getSmallestIntegerContainer());
        g.setColour(juce::Colour(0xFF636363).withAlpha(darkBlurAlpha));
        g.fillRoundedRectangle(darkBlurRect, darkBlurRect.getHeight() * 0.46f);
        g.setColour(juce::Colour(0xFFD9D8DB).withAlpha(lightBlurAlpha));
        g.fillRoundedRectangle(lightBlurRect, lightBlurRect.getHeight() * 0.48f);
        g.restoreState();

        g.setColour(juce::Colours::white.withAlpha(outerHighlightAlpha));
        g.fillRoundedRectangle(indent.translated(2.0f * us, 2.0f * us), indentR);

        g.setColour(juce::Colours::black.withAlpha(outerShadowAlpha));
        g.fillRoundedRectangle(indent.translated(-1.0f * us, -1.2f * us), indentR);

        juce::ColourGradient indGrad(juce::Colour(0xFFDADBDD), indent.getX(), indent.getCentreY(),
                                     juce::Colour(0xFFC8C9CB), indent.getRight(), indent.getCentreY(), false);
        g.setGradientFill(indGrad);
        g.fillRoundedRectangle(indent, indentR);

        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.drawRoundedRectangle(indent, indentR, us * 0.8f);

        g.setColour(juce::Colour(0xFF0F0F11));
        g.fillRoundedRectangle(cutout, cutoutR);

        g.setColour(juce::Colours::white.withAlpha(0.42f));
        g.drawRoundedRectangle(cutout, cutoutR, us * 0.75f);

        auto cutoutTopShade = juce::Rectangle<float>(cutout.getWidth() - 2.0f * us, 1.5f * us)
                                  .withPosition(cutout.getX() + 1.0f * us, cutout.getY() + 1.0f * us);
        auto cutoutBottomGlow = juce::Rectangle<float>(cutout.getWidth() - 2.0f * us, 1.5f * us)
                                    .withPosition(cutout.getX() + 1.0f * us, cutout.getBottom() - 2.5f * us);

        g.setColour(juce::Colours::black.withAlpha(cutoutTopShadeAlpha));
        g.fillRoundedRectangle(cutoutTopShade, cutoutTopShade.getHeight() * 0.5f);

        g.setColour(juce::Colours::white.withAlpha(cutoutBottomGlowAlpha));
        g.fillRoundedRectangle(cutoutBottomGlow, cutoutBottomGlow.getHeight() * 0.5f);

        auto centreShadow = juce::Rectangle<float>(cutout.getWidth() - 4.0f * us, 8.0f * us)
                                .withCentre(cutout.getCentre())
                                .translated(0.0f, innerLaneOffsetY * us);
        const float centreShadowR = centreShadow.getHeight() * 0.5f;

        g.setColour(juce::Colour(0xFF0B0C0E));
        g.fillRoundedRectangle(centreShadow, centreShadowR);

        auto topCore = juce::Rectangle<float>(centreShadow.getWidth(), 2.5f * us)
                           .withPosition(centreShadow.getX(), centreShadow.getY());
        auto bottomRim = juce::Rectangle<float>(centreShadow.getWidth(), 2.5f * us)
                             .withPosition(centreShadow.getX(), centreShadow.getBottom() - 2.5f * us);

        g.setColour(juce::Colours::black.withAlpha(coreTopAlpha));
        g.fillRoundedRectangle(topCore, topCore.getHeight() * 0.5f);

        g.setColour(juce::Colours::white.withAlpha(coreBottomAlpha));
        g.fillRoundedRectangle(bottomRim, bottomRim.getHeight() * 0.5f);
    };

    // Helper: draw CSS horizontal fader track, CLIPPED to coverRef
    auto drawHorizontalTrackClipped = [&](const juce::Rectangle<int>& faderRef,
                                          const juce::Rectangle<int>& cutoutRef,
                                          const juce::Rectangle<int>& clipRef)
    {
        auto clipArea = scaleRect(clipRef);
        g.saveState();
        g.reduceClipRegion(clipArea);

        auto indent = scaleRect(faderRef).toFloat();
        auto cutout = scaleRect(cutoutRef).toFloat();
        float indentR = indent.getHeight() * 0.45f;
        float cutoutR = cutout.getHeight() * 0.64f;

        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillRoundedRectangle(indent.translated(2.0f * us, 2.0f * us), indentR);
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(indent.translated(-1.0f * us, -1.5f * us), indentR);

        juce::ColourGradient indGrad(juce::Colour(0xFFBBBBBD), indent.getX(), indent.getCentreY(),
                                     juce::Colour(0xFFB1B1B3), indent.getRight(), indent.getCentreY(), false);
        g.setGradientFill(indGrad);
        g.fillRoundedRectangle(indent, indentR);
        g.setColour(juce::Colours::black.withAlpha(0.32f));
        g.drawRoundedRectangle(indent, indentR, us);

        g.setColour(juce::Colour(0xFF0F0F11));
        g.fillRoundedRectangle(cutout, cutoutR);
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawRoundedRectangle(cutout, cutoutR, us * 0.8f);

        g.restoreState();
    };

    // --- Cover static knobs and redraw only overlapping track portions ---

    // Left/right big faders: redraw full body from CSS instead of moving cleanup covers.
    drawVerticalTrackFull(kLeftFaderRef, kLeftFaderCutoutRef,
                          kLeftFaderBottomCleanupRef,
                          kLeftFaderBottomCleanupSampleRef,
                          true,
                          0.0f, 0.0f, 0.0f,
                          0.06f, 0.08f, 0.55f, 0.45f,
                          0.00f, 0.00f, 0.00f, 0.00f, 0.00f);
    {
        auto blobCoverArea = scaleRect(kRightFaderBottomBlobCoverRef).toFloat();
        blobCoverArea = blobCoverArea.translated(-2.0f * us, 4.0f * us);
        fillChassisBlendPatch(g, blobCoverArea, designScreenTop, designScreenBot);
    }
    drawVerticalTrackFull(kRightFaderRef, kRightFaderCutoutRef,
                          kRightFaderBottomCleanupRef,
                          kRightFaderBottomCleanupSampleRef,
                          true,
                          0.0f, 0.0f, 0.0f,
                          0.06f, 0.08f, 0.55f, 0.45f,
                          0.00f, 0.00f, 0.00f, 0.00f, 0.00f);

    {
        auto belowCleanupArea = scaleRect(kRightFaderBelowCleanupRef).toFloat();
        fillCleanupTexture(g, sideFaderBottomCleanPatch, belowCleanupArea, belowCleanupArea.getHeight() * 0.45f);
    }
    // ── DISTORTION FADERS — background blend + clipped track ─────────────────
    for (int i = 0; i < 3; ++i)
    {
        drawVerticalTrackFullSmall(kDistortionFaderRefs[(size_t)i],
                                   kDistortionCutoutRefs[(size_t)i],
                                   kDistKnobCoverRefs[(size_t)i],
                                   false,
                                   0.0f,   // bodyOffsetX
                                   0.0f,   // innerLaneOffsetX
                                   0.05f,  // darkBlurAlpha
                                   0.07f,  // lightBlurAlpha
                                   0.42f,  // outerHighlightAlpha
                                   0.30f,  // outerShadowAlpha
                                   0.22f,  // lowerShadowAlpha
                                   0.26f,  // cutoutLeftShadeAlpha
                                   0.14f,  // cutoutRightGlowAlpha
                                   0.18f,  // coreLeftAlpha
                                   0.10f); // coreRightAlpha
    }

    // ── WIDTH SLIDER — background blend + clipped track ───────────────────────
    drawHorizontalTrackFull(kWidthFaderRef,
                            kWidthCutoutRef,
                            kWidthKnobCoverRef,
                            false,
                            0.0f,   // bodyOffsetY
                            0.0f,   // innerLaneOffsetY
                            0.05f,  // darkBlurAlpha
                            0.07f,  // lightBlurAlpha
                            0.42f,  // outerHighlightAlpha
                            0.24f,  // outerShadowAlpha
                            0.24f,  // cutoutTopShadeAlpha
                            0.12f,  // cutoutBottomGlowAlpha
                            0.18f,  // coreTopAlpha
                            0.10f); // coreBottomAlpha
    }

// ============================================================================
// SIDE FADER THUMBS — single green sphere, moveable
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawSideFaderHandles(juce::Graphics& g)
{
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    auto draw = [&](const juce::Slider& s, const juce::Rectangle<int>& cutoutRef)
    {
        const auto lane = scaleRect(cutoutRef).toFloat();
        const float minV = (float) s.getMinimum();
        const float maxV = (float) s.getMaximum();
        const float t    = juce::jlimit(0.0f, 1.0f,
                               ((float)s.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));
        const float drawD = kSideFaderKnobOuterSize * us;
        const float drawHalfD = drawD * 0.5f;
        const float cy   = juce::jmap(t, lane.getBottom() - drawHalfD, lane.getY() + drawHalfD);
        const auto  knob = juce::Rectangle<float>(drawD, drawD)
                               .withCentre({ lane.getCentreX(), cy });

        drawGreenFaderKnob(g, knob);
    };

    draw(inputGainSlider, kLeftFaderCutoutRef);
    draw(outputGainSlider, kRightFaderCutoutRef);
}

// ============================================================================
// ROTARY KNOBS
// Draw ONLY the inner dome cap — the outer ring is already in bg_standard.png.
// capR = fullR * 0.58  (slightly smaller than before for tighter fit with ring)
// Gradient tone: #D8D8D8 → #A8A8A8 (muted silver matching the bg image)
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawSingleRotaryKnobOverlay(
    juce::Graphics& g, const juce::Slider& slider,
    const juce::Rectangle<int>& /*unused*/, bool isGreen, float /*unused*/)
{
    const auto area = slider.getBounds().toFloat();
    const float minV = (float) slider.getMinimum();
    const float maxV = (float) slider.getMaximum();
    const float norm = juce::jlimit(0.0f, 1.0f,
                           ((float)slider.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));

    const float fullR  = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const auto  centre = area.getCentre();

    const float capR = fullR * 0.47f;
    const juce::Rectangle<float> capRect(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

    auto shadow = capRect.translated(fullR * 0.03f, fullR * 0.08f)
                         .withSizeKeepingCentre(capRect.getWidth() * 0.92f, capRect.getHeight() * 0.38f);
    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillEllipse(shadow);

    if (isGreen)
    {
        juce::ColourGradient grad(juce::Colour(0xFF69D68A), centre.x, capRect.getY(),
                                  juce::Colour(0xFF2E9D56), centre.x, capRect.getBottom(), false);
        grad.addColour(0.50, juce::Colour(0xFF47B96C));
        g.setGradientFill(grad);
        g.fillEllipse(capRect);
    }
    else
    {
        juce::ColourGradient grad(juce::Colour(0xFFE9E9EB), centre.x, capRect.getY(),
                                  juce::Colour(0xFFC2C2C6), centre.x, capRect.getBottom(), false);
        grad.addColour(0.52, juce::Colour(0xFFD3D3D7));
        g.setGradientFill(grad);
        g.fillEllipse(capRect);
    }

    const float startAngle = juce::degreesToRadians(225.0f);
    const float endAngle   = juce::degreesToRadians(495.0f);
    const float angle      = juce::jmap(norm, startAngle, endAngle);
    const float dotOrbit = capR * 0.78f;
    const float dotR = capR * 0.06f;
    const float dotX       = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * dotOrbit;
    const float dotY       = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * dotOrbit;

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillEllipse(dotX - dotR + 0.6f, dotY - dotR + 0.8f, dotR * 2.0f, dotR * 2.0f);
    g.setColour(juce::Colour(0xFF5D5D60));
    g.fillEllipse(dotX - dotR, dotY - dotR, dotR*2.0f, dotR*2.0f);
}

void ThreeVoicesAudioProcessorEditor::drawRotaryKnobOverlays(juce::Graphics& g)
{
    for (int i = 0; i < 3; ++i)
    {
        drawSingleRotaryKnobOverlay(g, rows[i].speedKnob, {}, false);
        drawSingleRotaryKnobOverlay(g, rows[i].delayKnob, {}, false);
        drawSingleRotaryKnobOverlay(g, rows[i].depthKnob, {}, false);
    }
    drawSingleRotaryKnobOverlay(g, mixKnob, {}, true);
}

// ============================================================================
// DISTORTION SLIDER THUMBS — vertical silver capsule, matching bg_standard.png
// Width  = visual track width (48 dp) * 1.55  → slightly wider than the track
// Height = Width * 1.5                         → taller pill, portrait orientation
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawDistortionSliderHandles(juce::Graphics& g)
{
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    const float outerSize = kSmallFaderKnobOuterSize * us;
    const float halfSize = outerSize * 0.5f;

    for (int i = 0; i < 3; ++i)
    {
        const auto& s = rows[i].distortionSlider;
        const auto travel = scaleRect(kDistortionTravelRefs[(size_t) i]).toFloat();
        const auto cutout = scaleRect(kDistortionCutoutRefs[(size_t) i]).toFloat();

        const float minV = (float) s.getMinimum();
        const float maxV = (float) s.getMaximum();
        const float t = juce::jlimit(0.0f, 1.0f,
            ((float) s.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));

        const float topCy = cutout.getY() + halfSize;
        const float bottomCy = cutout.getBottom() - halfSize;
        const float cy = juce::jmap(t, bottomCy, topCy);

        // moved right and slightly down
        const auto thumbOffset = kDistortionThumbOffset[(size_t) i];
        const float cx = cutout.getCentreX() + thumbOffset.x * us;
        const float finalCy = cy + thumbOffset.y * us;

        const auto knob = juce::Rectangle<float>(outerSize, outerSize)
                              .withCentre({ cx, finalCy });

        drawSilverFaderKnob(g, knob);
    }
}

// ============================================================================
// WIDTH SLIDER THUMB — horizontal silver capsule matching bg_standard.png
// Height = visual track height (70 dp) * 0.85  → fits neatly inside the track
// Width  = Height * 1.6                          → landscape pill shape
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawWidthSliderOverlay(juce::Graphics& g)
{
    const float us = juce::jmin(getWidth() / (float) designW,
                                getHeight() / (float) designH);
    const float outerSize = kSmallFaderKnobOuterSize * us;
    const float halfSize = outerSize * 0.5f;

    const float t = juce::jlimit(0.0f, 1.0f,
        ((float) widthSlider.getValue() - (float) widthSlider.getMinimum()) /
        juce::jmax(0.0001f,
            (float) widthSlider.getMaximum() - (float) widthSlider.getMinimum()));

    const auto cutout = scaleRect(kWidthCutoutRef).toFloat();

    const float leftCx  = cutout.getX() + halfSize;
    const float rightCx = cutout.getRight() - halfSize;

    const float cx = juce::jmap(t, leftCx, rightCx) + kWidthThumbOffset.x * us;
    const float cy = cutout.getCentreY() + kWidthThumbOffset.y * us;

    drawSilverFaderKnob(g,
        juce::Rectangle<float>(outerSize, outerSize).withCentre({ cx, cy }));
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::updateScreenAnimationPlayback()
{
    if (screenVideo && !screenVideo->isPlaying()) screenVideo->play();
}

bool ThreeVoicesAudioProcessorEditor::advanceFallbackAnimationFrame()
{
    if (screenVideo != nullptr || animationFrames.isEmpty()) return false;
    currentAnimationFrame = (currentAnimationFrame + 1) % animationFrames.size();
    return true;
}

// ============================================================================
// Mouse forwarding — invisible linear sliders can't receive mouse events,
// so the parent editor intercepts and forwards to the correct slider.
// ============================================================================
juce::Slider* ThreeVoicesAudioProcessorEditor::findLinearSliderAt(juce::Point<int> pos)
{
    for (int i = 0; i < 3; ++i)
    {
        auto hit = scaleRect(kDistortionTravelRefs[(size_t) i]);
        hit = applyRectAdjust(hit, kDistortionSliderBoundsAdjust[(size_t) i]);
        if (hit.contains(pos))
            return &rows[(size_t) i].distortionSlider;
    }
    if (widthSlider.getBounds().contains(pos))      return &widthSlider;
    if (inputGainSlider.getBounds().contains(pos))  return &inputGainSlider;
    if (outputGainSlider.getBounds().contains(pos)) return &outputGainSlider;
    return nullptr;
}

void ThreeVoicesAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    activeDragSlider = findLinearSliderAt(e.getPosition());
    if (activeDragSlider)
        activeDragSlider->mouseDown(e.getEventRelativeTo(activeDragSlider));
}

void ThreeVoicesAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (activeDragSlider)
    {
        activeDragSlider->mouseDrag(e.getEventRelativeTo(activeDragSlider));
        repaint();
    }
}

void ThreeVoicesAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if (activeDragSlider)
    {
        activeDragSlider->mouseUp(e.getEventRelativeTo(activeDragSlider));
        activeDragSlider = nullptr;
        repaint();
    }
}

void ThreeVoicesAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,
                                                      const juce::MouseWheelDetails& w)
{
    if (auto* s = findLinearSliderAt(e.getPosition()))
        s->mouseWheelMove(e.getEventRelativeTo(s), w);
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::resized()
{
    // ── Knob row Y positions (design space) ──────────────────────────────────
    // bg_standard.png ring centres: row0≈y483, row1≈y968, row2≈y1454
    // component top = centre - halfSize(152) 
    const std::array<int, 3> knobRowY { 328,  816,  1302 };
    const std::array<int, 3> bitY     { 394,  882,  1370 };
    const std::array<int, 3> tubeY    { 502,  990,  1478 };

    for (int i = 0; i < 3; ++i)
    {
        auto& row = rows[i];
        row.voiceButton.setBounds      (scaleRect({ 307,  291 + i*483,   283, 378 }));

        // Knob bounds shifted left ~30 dp to align caps over bg rings
        row.speedKnob.setBounds        (scaleRect({ 676,  knobRowY[i],   304, 304 }));
        row.delayKnob.setBounds        (scaleRect({ 1058, knobRowY[i],   304, 304 }));
        row.depthKnob.setBounds        (scaleRect({ 1416, knobRowY[i],   304, 304 }));

        // Distortion: component centred on visual track (shifted left with knobs)
        auto distBounds = scaleRect(kDistortionTravelRefs[i]);
        distBounds = applyRectAdjust(distBounds, kDistortionSliderBoundsAdjust[(size_t) i]);
        row.distortionSlider.setBounds(distBounds);
        row.bitButton.setBounds        (scaleRect({ 1988, bitY[i],       171,  70 }));
        row.tubeButton.setBounds       (scaleRect({ 1988, tubeY[i],      171,  70 }));
    }

    // Width: shifted left ~40 dp and up ~30 dp
    auto widthBounds = scaleRect(kWidthCutoutRef);
    widthBounds = applyRectAdjust(widthBounds, kWidthSliderBoundsAdjust);
    widthSlider.setBounds(widthBounds);

    // Mix knob: shifted left ~100 dp to align over ring in bg
    mixKnob.setBounds          (scaleRect({ 2618, 1451,  315, 315  }));
    inputGainSlider.setBounds  (scaleRect({   38,  167,  244, 1618 }));
    outputGainSlider.setBounds (scaleRect({ 2800,  167,  344, 1618 }));
    presetButton.setBounds     (scaleRect(kPresetOpenRef));
    previousPresetButton.setBounds(scaleRect(kPresetPrevRef));
    nextPresetButton.setBounds (scaleRect(kPresetNextRef));

    if (screenVideo != nullptr)
    {
        screenVideo->setBounds(scaleRect(kVideoRef));
        screenVideo->toBack();
    }
    if (presetOverlay != nullptr)
        presetOverlay->setBounds(getLocalBounds());
}

// ============================================================================
void ThreeVoicesAudioProcessorEditor::openPresetOverlay()
{
    if (presetOverlay == nullptr)
    {
        presetOverlay = std::make_unique<PresetMenuOverlay>();
        presetOverlay->setMenuImages(&menuCategories, &menuClassicMod, &menuGuitar,
                                     &menuKeysSynth,  &menuBass,       &menuDrums, &menuVocals);
        presetOverlay->setCallbacks(
            [this](int cat, int pre) { onOverlayPresetSelected(cat, pre); },
            [this]()                 { closePresetOverlay(); });
        addAndMakeVisible(*presetOverlay);
        presetOverlay->setBounds(getLocalBounds());
    }
    presetOverlay->setPresetLibrary(audioProcessor.getFlattenedPresetChoices());
    presetOverlay->openCategories();
}

void ThreeVoicesAudioProcessorEditor::closePresetOverlay()
{
    if (presetOverlay != nullptr) presetOverlay->setVisible(false);
    repaint();
}

void ThreeVoicesAudioProcessorEditor::stepPreset(int delta)
{
    const auto& choices = audioProcessor.getFlattenedPresetChoices();
    if (choices.isEmpty())
        return;

    const int current = audioProcessor.getCurrentPresetIndex();
    const int next = juce::jlimit(0, choices.size() - 1, current + delta);
    if (next == current)
        return;

    audioProcessor.setCurrentPresetIndex(next);
    audioProcessor.applyImageDerivedPreset(next);
    cachedPresetName = getCurrentPresetName();
    repaint(scaleRect(kPresetOpenRef));
    repaint(scaleRect(kScreenBodyRef));
}

void ThreeVoicesAudioProcessorEditor::onOverlayPresetSelected(int cat, int pre)
{
    const auto& choices = audioProcessor.getFlattenedPresetChoices();
    if (choices.isEmpty())
        return;

    juce::String currentCategory;
    int currentCategoryIndex = -1;
    int absoluteIndex = -1;
    int categoryCounter = -1;

    for (int i = 0; i < choices.size(); ++i)
    {
        const auto separator = choices[i].indexOf(" - ");
        if (separator < 0)
            continue;

        const auto category = choices[i].substring(0, separator).trim();
        if (category != currentCategory)
        {
            currentCategory = category;
            ++categoryCounter;
            currentCategoryIndex = 0;
        }
        else
        {
            ++currentCategoryIndex;
        }

        if (categoryCounter == cat && currentCategoryIndex == pre)
        {
            absoluteIndex = i;
            break;
        }
    }

    if (absoluteIndex < 0)
        return;

    audioProcessor.setCurrentPresetIndex(absoluteIndex);
    audioProcessor.applyImageDerivedPreset(absoluteIndex);
    cachedPresetName = getCurrentPresetName();
    repaint();
}

void ThreeVoicesAudioProcessorEditor::timerCallback()
{
    updateScreenAnimationPlayback();
    if (advanceFallbackAnimationFrame())
        repaint(scaleRect(kScreenBodyRef));
    const auto current = getCurrentPresetName();
    if (current != cachedPresetName) { cachedPresetName = current; repaint(); }
}

void ThreeVoicesAudioProcessorEditor::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync(
        [safe = juce::Component::SafePointer<ThreeVoicesAudioProcessorEditor>(this)]
        { if (safe != nullptr) safe->repaint(); });
}
  
