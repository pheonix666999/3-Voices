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

const juce::Rectangle<int> kScreenBodyRef { 2272, 724,  620, 637 };
const juce::Rectangle<int> kVideoRef      { 2330, 750,  580, 590 };
const juce::Rectangle<int> kPresetTextRef { 2458, 566, 432, 58 };
const juce::Rectangle<int> kPresetOpenRef { 2438, 533, 470, 126 };
const juce::Rectangle<int> kPresetPrevRef { 2888, 552, 64, 38 };
const juce::Rectangle<int> kPresetNextRef { 2888, 596, 64, 38 };
const juce::Rectangle<int> kLeftFaderRef  { 134, 201, 71, 1543 };
const juce::Rectangle<int> kRightFaderRef { 3161, 204, 71, 1543 };
const juce::Rectangle<int> kLeftFaderCutoutRef  { 149, 213, 41, 1500 };
const juce::Rectangle<int> kRightFaderCutoutRef { 3176, 216, 41, 1500 };
const juce::Rectangle<int> kLeftFaderStraightPatchRef  { 118, 624, 103, 142 };
const juce::Rectangle<int> kRightFaderStraightPatchRef { 3145, 624, 103, 142 };
const juce::Rectangle<int> kLeftFaderTopPatchRef  { 134, 201, 71, 160 };   // Clean top rail strip to reuse at the bottom.
const juce::Rectangle<int> kRightFaderTopPatchRef { 3161, 204, 71, 160 };  // Clean top rail strip to reuse at the bottom.
const juce::Rectangle<int> kLeftFaderBottomPatchRef  { 118, 1562, 103, 176 };   // Lower redraw area for the left big slider.
const juce::Rectangle<int> kRightFaderBottomPatchRef { 3145, 1562, 103, 176 };  // Lower redraw area for the right big slider.
const juce::Rectangle<int> kInputFaderStemCleanupRef       { 151, 1514,  30, 142 };
const juce::Rectangle<int> kOutputFaderStemCleanupRef      { 3080, 1514, 30, 142 };
const juce::Rectangle<int> kInputFaderStemCleanupSampleRef { 138, 1324,  56, 158 };
const juce::Rectangle<int> kOutputFaderStemCleanupSampleRef{ 3067, 1324, 56, 158 };
const juce::Rectangle<int> kInputFaderCapCleanupRef        { 116, 1608, 100,  76 };
const juce::Rectangle<int> kOutputFaderCapCleanupRef       { 3045, 1608, 100,  76 };
const juce::Rectangle<int> kInputFaderCapCleanupSampleRef  { 106, 1508, 120,  86 };
const juce::Rectangle<int> kOutputFaderCapCleanupSampleRef { 3035, 1508, 120,  86 };
const juce::Rectangle<int> kLeftFaderBottomCleanupRef      { 108, 1540, 112, 126 };
const juce::Rectangle<int> kRightFaderBottomCleanupRef     { 3037, 1540, 112, 126 };
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
    { 1876,  343, 48, 258 },
    { 1876,  831, 48, 258 },
    { 1876, 1319, 48, 258 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionFaderRefs {{
    { 1895,  343, 51, 300 },
    { 1895,  831, 51, 300 },
    { 1898, 1319, 51, 300 }
}};

const std::array<juce::Rectangle<int>, 3> kDistortionCutoutRefs {{
    { 1908,  358, 25, 269 },
    { 1908,  846, 25, 269 },
    { 1911, 1334, 25, 269 }
}};

const juce::Rectangle<int> kWidthInnerTrackRef { 2271, 1579, 148, 30 };

// Visual track widths/heights from bg_standard.png (design px)
// These drive thumb sizing, independent of the wider hit-area components.
constexpr float kSideFaderKnobOuterSize = 108.895f;
constexpr float kSideFaderKnobTopSize = 77.782f;
constexpr float kSmallFaderKnobOuterSize = 63.0f;
constexpr float kSmallFaderKnobTopSize = 45.0f;

juce::String sanitisePresetDisplayName(juce::String name)
{
    name = name.trim();
    while (name.contains("  "))
        name = name.replace("  ", " ");
    name = name.replace("_", " ");
    return name.trim();
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
    sideFaderThumb = loadImageByName("fader_grip.png");
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
        if (d && sz > 0) return juce::ImageFileFormat::loadFrom(d, (size_t) sz);
        return {};
    };
    if (auto img = fromBinary(); img.isValid()) return img;

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
        const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
        const int w = (int) std::round(designW * us), h = (int) std::round(designH * us);
        g.drawImageWithin(bgStandard, (getWidth()-w)/2, (getHeight()-h)/2, w, h,
                          juce::RectanglePlacement::stretchToFit, false);
    }
    else { g.fillAll(juce::Colour(0xFFBFC0C2)); }

    drawRotaryKnobOverlays(g);
    drawToggleOverlays(g);
    drawScreenFallbackAnimation(g);
    drawPresetDisplay(g);
}

void ThreeVoicesAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    drawSideFaderHandles(g);
    drawDistortionSliderHandles(g);
    drawWidthSliderOverlay(g);
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
    const float rim = outerRect.getWidth() * 0.5f;
    const auto topRect = outerRect.withSizeKeepingCentre(outerRect.getWidth() * (kSideFaderKnobTopSize / kSideFaderKnobOuterSize),
                                                         outerRect.getHeight() * (kSideFaderKnobTopSize / kSideFaderKnobOuterSize));
    const auto baseShadow = outerRect.translated(0.0f, outerRect.getHeight() * 0.22f)
                                     .withSizeKeepingCentre(outerRect.getWidth() * 0.92f, outerRect.getHeight() * 0.58f);

    g.setColour(juce::Colours::black.withAlpha(0.36f));
    g.fillEllipse(baseShadow);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(baseShadow.translated(0.0f, outerRect.getHeight() * 0.015f), outerRect.getWidth() * 0.014f);

    juce::ColourGradient rimGrad(juce::Colour(0xFF62C985), outerRect.getCentreX(), outerRect.getY(),
                                 juce::Colour(0xFF2C9B58), outerRect.getCentreX(), outerRect.getBottom(), false);
    rimGrad.addColour(0.55, juce::Colour(0xFF49B96D));
    g.setGradientFill(rimGrad);
    g.fillEllipse(outerRect);

    juce::ColourGradient topGrad(juce::Colour(0xFF5FCD84), topRect.getCentreX(), topRect.getY(),
                                 juce::Colour(0xFF42B066), topRect.getCentreX(), topRect.getBottom(), false);
    topGrad.addColour(0.60, juce::Colour(0xFF4CBD71));
    g.setGradientFill(topGrad);
    g.fillEllipse(topRect);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(outerRect.reduced(outerRect.getWidth() * 0.02f), outerRect.getWidth() * 0.014f);
    g.setColour(juce::Colours::black.withAlpha(0.16f));
    g.drawEllipse(topRect.reduced(topRect.getWidth() * 0.02f), topRect.getWidth() * 0.012f);
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    g.drawEllipse(outerRect.reduced(outerRect.getWidth() * 0.10f, outerRect.getHeight() * 0.10f), rim * 0.72f * 0.02f);
}

static void drawSilverFaderKnob(juce::Graphics& g, const juce::Rectangle<float>& outerRect)
{
    const float outerR = outerRect.getWidth() * 0.5f;
    const auto topRect = outerRect.withSizeKeepingCentre(outerRect.getWidth() * (kSmallFaderKnobTopSize / kSmallFaderKnobOuterSize),
                                                         outerRect.getHeight() * (kSmallFaderKnobTopSize / kSmallFaderKnobOuterSize));
    const auto dropShadow = outerRect.translated(outerRect.getWidth() * 0.06f, outerRect.getHeight() * 0.16f)
                                     .withSizeKeepingCentre(outerRect.getWidth() * 1.08f, outerRect.getHeight() * 0.86f);

    g.setColour(juce::Colours::black.withAlpha(0.34f));
    g.fillEllipse(dropShadow);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawEllipse(dropShadow.translated(0.0f, outerRect.getHeight() * 0.015f), outerRect.getWidth() * 0.012f);

    juce::ColourGradient outerGrad(juce::Colour(0xFFF2F2F4), outerRect.getCentreX(), outerRect.getY(),
                                   juce::Colour(0xFFC7C7CB), outerRect.getCentreX(), outerRect.getBottom(), false);
    outerGrad.addColour(0.62, juce::Colour(0xFFD9D9DC));
    g.setGradientFill(outerGrad);
    g.fillEllipse(outerRect);

    juce::ColourGradient topGrad(juce::Colour(0xFFE5E5E8), topRect.getCentreX(), topRect.getY(),
                                 juce::Colour(0xFFC8C8CB), topRect.getCentreX(), topRect.getBottom(), false);
    topGrad.addColour(0.72, juce::Colour(0xFFD7D7DA));
    g.setGradientFill(topGrad);
    g.fillEllipse(topRect);

    g.setColour(juce::Colours::white.withAlpha(0.24f));
    g.drawEllipse(outerRect.reduced(outerRect.getWidth() * 0.02f), outerRect.getWidth() * 0.012f);
    g.setColour(juce::Colours::black.withAlpha(0.16f));
    g.drawEllipse(topRect.reduced(topRect.getWidth() * 0.02f), topRect.getWidth() * 0.012f);
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

// ============================================================================
// SIDE FADER THUMBS — single green sphere, moveable
// ============================================================================
void ThreeVoicesAudioProcessorEditor::drawSideFaderHandles(juce::Graphics& g)
{
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    auto drawPatch = [this, &g, us](const juce::Rectangle<int>& srcRef, const juce::Rectangle<float>& dstArea)
    {
        drawPatchFromImageWithBottomCurve(g, bgStandard, srcRef, dstArea, designW, designH, 26.0f * us);
    };

    drawPatch(kLeftFaderStraightPatchRef, scaleRect(kLeftFaderBottomPatchRef).toFloat());
    drawPatch(kRightFaderStraightPatchRef, scaleRect(kRightFaderBottomPatchRef).toFloat());

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
    const auto  area = slider.getBounds().toFloat();
    const float minV = (float) slider.getMinimum();
    const float maxV = (float) slider.getMaximum();
    const float norm = juce::jlimit(0.0f, 1.0f,
                           ((float)slider.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));

    const float fullR  = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const auto  centre = area.getCentre();

    // Keep the cap close to the original inner-dome size.
    const float capR = fullR * 0.515f;

    if (isGreen)
    {
        juce::ColourGradient grad(juce::Colour(0xFF56C97D),
                                  centre.x, centre.y - capR,
                                  juce::Colour(0xFF45B66B),
                                  centre.x, centre.y + capR, false);
        grad.addColour(0.75, juce::Colour(0xFF40AF66));
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

        for (int i = 0; i < 16; ++i)
        {
            const float arcT = i / 15.0f;
            const float arcAngle = juce::degreesToRadians(208.0f + 88.0f * arcT);
            const float notchR = capR * 0.92f;
            const float notchSize = capR * 0.060f;
            const float nx = centre.x + std::cos(arcAngle) * notchR;
            const float ny = centre.y + std::sin(arcAngle) * notchR;
            g.setColour(juce::Colours::black.withAlpha(0.11f));
            g.fillEllipse(nx - notchSize, ny - notchSize * 0.72f, notchSize * 2.0f, notchSize * 1.35f);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawEllipse(nx - notchSize * 0.72f, ny - notchSize * 0.48f,
                          notchSize * 1.44f, notchSize * 0.92f, 0.45f);
        }

        g.setColour(juce::Colours::white.withAlpha(0.16f));
        g.drawEllipse(centre.x - capR * 0.93f, centre.y - capR * 0.93f,
                      capR * 1.86f, capR * 1.86f, 0.55f);
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.drawEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f, 0.8f);
    }
    else
    {
        juce::ColourGradient grad(juce::Colour(0xFFD9D9DC),
                                  centre.x, centre.y - capR,
                                  juce::Colour(0xFFC7C7CA),
                                  centre.x, centre.y + capR, false);
        grad.addColour(0.78, juce::Colour(0xFFC3C3C7));
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

        for (int i = 0; i < 24; ++i)
        {
            const float arcT = i / 23.0f;
            const float arcAngle = juce::degreesToRadians(158.0f + 224.0f * arcT);
            const float notchR = capR * 0.98f;
            const float notchSize = capR * 0.038f;
            const float nx = centre.x + std::cos(arcAngle) * notchR;
            const float ny = centre.y + std::sin(arcAngle) * notchR;
            g.setColour(juce::Colours::black.withAlpha(0.10f));
            g.fillEllipse(nx - notchSize, ny - notchSize * 0.72f, notchSize * 2.0f, notchSize * 1.35f);
            g.setColour(juce::Colours::white.withAlpha(0.16f));
            g.drawEllipse(nx - notchSize * 0.72f, ny - notchSize * 0.48f,
                          notchSize * 1.44f, notchSize * 0.92f, 0.45f);
        }

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawEllipse(centre.x - capR * 0.93f, centre.y - capR * 0.93f,
                      capR * 1.86f, capR * 1.86f, 0.55f);
        g.setColour(juce::Colours::black.withAlpha(0.10f));
        g.drawEllipse(centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f, 0.8f);
    }

    // ── Indicator dot ────────────────────────────────────────────────────────
    const float startAngle = juce::degreesToRadians(225.0f);
    const float endAngle   = juce::degreesToRadians(495.0f);
    const float angle      = juce::jmap(norm, startAngle, endAngle);
    const float dotOrbit   = capR * 0.74f;
    const float dotR       = capR * 0.052f;
    const float dotX       = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * dotOrbit;
    const float dotY       = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * dotOrbit;

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(dotX - dotR + 0.4f, dotY - dotR + 0.6f, dotR*2.0f, dotR*2.0f);
    g.setColour(isGreen ? juce::Colour(0xFF0B2E16) : juce::Colour(0xFF1A1A1A));
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
    auto drawCleanup = [this, &g](const juce::Rectangle<int>& dstRef, float radiusScale)
    {
        const auto dst = scaleRect(dstRef).toFloat();
        fillCleanupTexture(g, sideFaderBottomCleanPatch, dst,
                           juce::jmin(dst.getWidth(), dst.getHeight()) * radiusScale);
    };

    for (int i = 0; i < 3; ++i)
    {
        const auto& s    = rows[i].distortionSlider;
        const auto  cutout = scaleRect(kDistortionCutoutRefs[(size_t) i]).toFloat();
        const float minV = (float) s.getMinimum();
        const float maxV = (float) s.getMaximum();
        const float t    = juce::jlimit(0.0f, 1.0f,
                               ((float)s.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));
        drawCleanup(kDistortionCleanupRefs[(size_t) i], 0.42f);
        const float cy   = juce::jmap(t, cutout.getBottom() - halfSize, cutout.getY() + halfSize);
        const auto  knob = juce::Rectangle<float>(outerSize, outerSize)
                               .withCentre(juce::Point<float>(cutout.getCentreX(), cy));
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
    const float us = juce::jmin(getWidth() / (float) designW, getHeight() / (float) designH);
    const float outerSize = kSmallFaderKnobOuterSize * us;
    const float halfW    = outerSize * 0.5f;
    auto drawCleanup = [this, &g](const juce::Rectangle<int>& dstRef, float radiusScale)
    {
        const auto dst = scaleRect(dstRef).toFloat();
        fillCleanupTexture(g, sideFaderBottomCleanPatch, dst,
                           juce::jmin(dst.getWidth(), dst.getHeight()) * radiusScale);
    };

    const float minV  = (float) widthSlider.getMinimum();
    const float maxV  = (float) widthSlider.getMaximum();
    const float t     = juce::jlimit(0.0f, 1.0f,
                             ((float)widthSlider.getValue() - minV) / juce::jmax(0.0001f, maxV - minV));

    const auto cutout = scaleRect(kWidthInnerTrackRef).toFloat();
    const float cx = juce::jmap(t, cutout.getX() + halfW, cutout.getRight() - halfW);
    drawCleanup(kWidthThumbCleanupRef, 0.38f);
    const auto knob = juce::Rectangle<float>(outerSize, outerSize)
                         .withCentre(juce::Point<float>(cx, cutout.getCentreY()));
    drawSilverFaderKnob(g, knob);
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
    for (auto& row : rows)
        if (row.distortionSlider.getBounds().contains(pos)) return &row.distortionSlider;
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
        row.distortionSlider.setBounds (scaleRect({ 1818, knobRowY[i],   160, 312 }));

        row.bitButton.setBounds        (scaleRect({ 1988, bitY[i],       171,  70 }));
        row.tubeButton.setBounds       (scaleRect({ 1988, tubeY[i],      171,  70 }));
    }

    // Width: shifted left ~40 dp and up ~30 dp
    widthSlider.setBounds      (scaleRect({ 2255, 1523,  355, 134  }));

    // Mix knob: shifted left ~100 dp to align over ring in bg
    mixKnob.setBounds          (scaleRect({ 2618, 1451,  315, 315  }));
    inputGainSlider.setBounds  (scaleRect({   38,  167,  244, 1618 }));
    outputGainSlider.setBounds (scaleRect({ 2984,  167,  244, 1618 }));
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
  
