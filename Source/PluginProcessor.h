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

    // Voice processing
    struct VoiceProcessor
    {
        float phase = 0.0f;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine{ 192000 }; // Support up to 192kHz

        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            // Set max delay to 100ms at current sample rate
            delayLine.setMaximumDelayInSamples(static_cast<int>(spec.sampleRate * 0.1));
            delayLine.prepare(spec);
        }

        void reset()
        {
            phase = 0.0f;
            delayLine.reset();
        }
    };

    VoiceProcessor voices[3];
    double currentSampleRate = 44100.0;

    // Parameter smoothing
    juce::SmoothedValue<float> smoothedInputGain;
    juce::SmoothedValue<float> smoothedOutputGain;
    juce::SmoothedValue<float> smoothedMix;
    juce::SmoothedValue<float> smoothedWidth;

    // Distortion processing
    float processTubeDistortion(float sample, float drive);
    float processBitCrusher(float sample, float drive);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeVoicesAudioProcessor)
};
