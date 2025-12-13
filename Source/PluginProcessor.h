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
        float phaseOffset = 0.0f; // For voice character differences
        float speedMultiplier = 1.0f; // For voice character differences
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine{ 28800 }; // Max 150ms at 192kHz (28800 samples max needed)
        juce::dsp::IIR::Filter<float> filter; // For subtle tone differences

        void prepare(const juce::dsp::ProcessSpec& spec, int voiceIndex)
        {
            // Set max delay to 150ms at current sample rate
            delayLine.setMaximumDelayInSamples(static_cast<int>(spec.sampleRate * 0.15));
            delayLine.prepare(spec);

            // Set up subtle character differences for each voice
            // Voice I: slight high shelf boost
            // Voice II: slight low shelf boost
            // Voice III: slight band pass character
            juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
            if (voiceIndex == 0)
            {
                // Voice I: slight high frequency emphasis
                coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                    spec.sampleRate, 8000.0f, 0.707f, 1.2f);
                phaseOffset = 0.0f;
                speedMultiplier = 1.0f;
            }
            else if (voiceIndex == 1)
            {
                // Voice II: slight low frequency emphasis
                coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                    spec.sampleRate, 200.0f, 0.707f, 1.15f);
                phaseOffset = 0.33f; // 120 degree phase offset
                speedMultiplier = 0.98f; // Slightly slower
            }
            else // voiceIndex == 2
            {
                // Voice III: slight mid emphasis
                coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                    spec.sampleRate, 2000.0f, 1.5f, 1.1f);
                phaseOffset = 0.67f; // 240 degree phase offset
                speedMultiplier = 1.02f; // Slightly faster
            }
            
            filter.coefficients = coeffs;
            filter.prepare(spec);
        }

        void reset()
        {
            phase = phaseOffset; // Start at phase offset
            delayLine.reset();
            filter.reset();
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
