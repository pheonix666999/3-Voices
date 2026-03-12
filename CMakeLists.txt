cmake_minimum_required(VERSION 3.22)

project(ThreeVoicesUnisonMod VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE - adjust the path to your JUCE installation
add_subdirectory(JUCE)

juce_add_binary_data(ThreeVoicesAssets
    SOURCES
        bg_standard.png
        bg_buttons_on.png
        menu_categories.png
        menu_classic_modulation.png
        menu_guitar.png
        menu_keys_synth.png
        menu_bass.png
        menu_vocals.png)

juce_add_plugin(ThreeVoices
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Yoco
    PLUGIN_CODE 3Vum
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "3 Voice Unison Mod")

target_sources(ThreeVoices
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/ui/UnisonLookAndFeel.cpp
        Source/ui/InvisibleLookAndFeel.cpp
        Source/ui/PresetMenuOverlay.cpp)

target_compile_definitions(ThreeVoices
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ThreeVoices
    PRIVATE
        ThreeVoicesAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_video
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
