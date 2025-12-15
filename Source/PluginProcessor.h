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
        // Use Lagrange interpolation for smooth delay changes (click-free)
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine{ 28800 }; // Max 150ms at 192kHz
        juce::dsp::IIR::Filter<float> filter; // For subtle tone differences
        // Smoothed delay time in samples (per-voice, updated per-sample)
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelaySamples;

        void prepare(const juce::dsp::ProcessSpec& spec, int voiceIndex)
        {
            // Set max delay to 150ms at current sample rate (+ safety margin)
            int maxDelaySamples = static_cast<int>(spec.sampleRate * 0.15f) + 64;
            delayLine.setMaximumDelayInSamples(maxDelaySamples);
            delayLine.prepare(spec);
            
            // Initialize smoothed delay time (10ms smoothing for delay changes)
            smoothedDelaySamples.reset(spec.sampleRate, 0.01);
            smoothedDelaySamples.setCurrentAndTargetValue(0.0f);

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

        void reset(double sampleRate)
        {
            phase = phaseOffset; // Start at phase offset
            delayLine.reset();
            filter.reset();
            smoothedDelaySamples.reset(sampleRate, 0.01); // 10ms smoothing for delay changes
            smoothedDelaySamples.setCurrentAndTargetValue(0.0f);
        }
    };

    VoiceProcessor voices[3];
    double currentSampleRate = 44100.0;

    // Parameter smoothing - all continuous parameters must be smoothed
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedInputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOutputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedWidth;
    
    // Per-voice parameter smoothers (3 voices)
    struct VoiceSmoothers
    {
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> speed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTime; // in samples
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depth;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> distortion;
    };
    VoiceSmoothers voiceSmoothers[3];

    // Distortion processing
    float processTubeDistortion(float sample, float drive);
    float processBitCrusher(float sample, float drive);
    
    // Constant-power panning helper (smooth, click-free panning)
    void applyConstantPowerPan(float& left, float& right, float panPosition);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeVoicesAudioProcessor)
};
