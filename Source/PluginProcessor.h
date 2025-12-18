#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class ThreeVoicesAudioProcessor : public juce::AudioProcessor
{
public:
    ThreeVoicesAudioProcessor();
    ~ThreeVoicesAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Voice processing - mono delay line with Lagrange interpolation
    struct VoiceProcessor
    {
        float phase = 0.0f;
        // Lagrange interpolation for smooth, click-free delay changes
        // Single channel - we process mono and create stereo via panning
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine{ 32000 };

        void prepare(const juce::dsp::ProcessSpec& spec, int /*voiceIndex*/)
        {
            // Max delay: 170ms at current sample rate (150ms + modulation headroom)
            int maxDelaySamples = static_cast<int>(spec.sampleRate * 0.17f) + 64;
            delayLine.setMaximumDelayInSamples(maxDelaySamples);

            // Prepare as mono (1 channel) - stereo is handled via panning
            juce::dsp::ProcessSpec monoSpec = spec;
            monoSpec.numChannels = 1;
            delayLine.prepare(monoSpec);
        }

        void reset(double /*sampleRate*/)
        {
            phase = 0.0f;
            delayLine.reset();
        }
    };

    VoiceProcessor voices[3];
    double currentSampleRate = 44100.0;

    // Parameter smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedInputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOutputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedWidth;

    // Per-voice parameter smoothers
    struct VoiceSmoothers
    {
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> speed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTime; // in ms
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depth;     // percentage
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> distortion;
    };
    VoiceSmoothers voiceSmoothers[3];

    // Distortion processing
    float processTubeDistortion(float sample, float drive);
    float processBitCrusher(float sample, float drive); // Now "Dirt" - warm overdrive

    // Panning helper
    void applyConstantPowerPan(float& left, float& right, float panPosition);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeVoicesAudioProcessor)
};
