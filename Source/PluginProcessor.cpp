#include "PluginProcessor.h"
#include "PluginEditor.h"

ThreeVoicesAudioProcessor::ThreeVoicesAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

ThreeVoicesAudioProcessor::~ThreeVoicesAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ThreeVoicesAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input/Output Gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("inputGain", 1), "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("outputGain", 1), "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    // Global Mix and Width
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("width", 1), "Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    // Voice parameters
    for (int i = 1; i <= 3; ++i)
    {
        juce::String prefix = "voice" + juce::String(i);

        // Voice On/Off
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(prefix + "On", 1), "Voice " + juce::String(i) + " On", false));

        // Speed (LFO rate in Hz)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "Speed", 1), "Voice " + juce::String(i) + " Speed",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));

        // Delay Time (ms)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "DelayTime", 1), "Voice " + juce::String(i) + " Delay Time",
            juce::NormalisableRange<float>(1.0f, 50.0f, 0.1f), 10.0f));

        // Depth (modulation depth %)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "Depth", 1), "Voice " + juce::String(i) + " Depth",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

        // Distortion Amount
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "Distortion", 1), "Voice " + juce::String(i) + " Distortion",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

        // Tube On/Off
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(prefix + "Tube", 1), "Voice " + juce::String(i) + " Tube", false));

        // Bit Crusher On/Off
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(prefix + "Bit", 1), "Voice " + juce::String(i) + " Bit", false));
    }

    return { params.begin(), params.end() };
}

const juce::String ThreeVoicesAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ThreeVoicesAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ThreeVoicesAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ThreeVoicesAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ThreeVoicesAudioProcessor::getTailLengthSeconds() const
{
    // Return max delay time (50ms) plus some buffer
    return 0.1;
}

int ThreeVoicesAudioProcessor::getNumPrograms()
{
    return 1;
}

int ThreeVoicesAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ThreeVoicesAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ThreeVoicesAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ThreeVoicesAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void ThreeVoicesAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    for (int i = 0; i < 3; ++i)
    {
        voices[i].prepare(spec);
        voices[i].reset();
    }

    // Initialize parameter smoothing (20ms smoothing time)
    smoothedInputGain.reset(sampleRate, 0.02);
    smoothedOutputGain.reset(sampleRate, 0.02);
    smoothedMix.reset(sampleRate, 0.02);
    smoothedWidth.reset(sampleRate, 0.02);

    // Set initial values
    smoothedInputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load()));
    smoothedOutputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    smoothedMix.setCurrentAndTargetValue(apvts.getRawParameterValue("mix")->load() * 0.01f);
    smoothedWidth.setCurrentAndTargetValue(apvts.getRawParameterValue("width")->load() * 0.01f);
}

void ThreeVoicesAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ThreeVoicesAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

float ThreeVoicesAudioProcessor::processTubeDistortion(float sample, float drive)
{
    // Soft tube saturation using tanh waveshaping
    float amount = drive * 0.01f * 4.0f; // Scale drive for more noticeable effect
    if (amount < 0.01f) return sample;

    // Apply gain before saturation, then normalize
    float x = sample * (1.0f + amount * 2.0f);
    return std::tanh(x) / std::tanh(1.0f + amount * 2.0f);
}

float ThreeVoicesAudioProcessor::processBitCrusher(float sample, float drive)
{
    // Bit crusher effect - reduces bit depth
    float amount = drive * 0.01f;
    if (amount < 0.01f) return sample;

    // Map drive to bit depth: 0% = 16 bits, 100% = 2 bits
    float bits = juce::jmap(amount, 0.0f, 1.0f, 16.0f, 2.0f);
    float levels = std::pow(2.0f, bits);

    // Quantize the signal
    return std::round(sample * levels) / levels;
}

void ThreeVoicesAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update smoothed parameter targets
    smoothedInputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load()));
    smoothedOutputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    smoothedMix.setTargetValue(apvts.getRawParameterValue("mix")->load() * 0.01f);
    smoothedWidth.setTargetValue(apvts.getRawParameterValue("width")->load() * 0.01f);

    // Voice parameters (these change less frequently, so we read once per block)
    bool voiceOn[3];
    float voiceSpeed[3], voiceDelayTime[3], voiceDepth[3], voiceDistortion[3];
    bool voiceTube[3], voiceBit[3];

    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "voice" + juce::String(i + 1);
        voiceOn[i] = apvts.getRawParameterValue(prefix + "On")->load() > 0.5f;
        voiceSpeed[i] = apvts.getRawParameterValue(prefix + "Speed")->load();
        voiceDelayTime[i] = apvts.getRawParameterValue(prefix + "DelayTime")->load();
        voiceDepth[i] = apvts.getRawParameterValue(prefix + "Depth")->load() * 0.01f;
        voiceDistortion[i] = apvts.getRawParameterValue(prefix + "Distortion")->load();
        voiceTube[i] = apvts.getRawParameterValue(prefix + "Tube")->load() > 0.5f;
        voiceBit[i] = apvts.getRawParameterValue(prefix + "Bit")->load() > 0.5f;
    }

    // Store dry signal before processing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Apply input gain to dry buffer (smoothed per-sample in the loop)
    // We'll apply it sample-by-sample below

    // Process each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Get smoothed parameter values for this sample
        float inputGain = smoothedInputGain.getNextValue();
        float outputGain = smoothedOutputGain.getNextValue();
        float mix = smoothedMix.getNextValue();
        float width = smoothedWidth.getNextValue();

        // Get input samples with input gain applied
        float inputL = dryBuffer.getSample(0, sample) * inputGain;
        float inputR = dryBuffer.getNumChannels() > 1 ? dryBuffer.getSample(1, sample) * inputGain : inputL;

        float wetL = 0.0f;
        float wetR = 0.0f;
        int activeVoices = 0;

        for (int v = 0; v < 3; ++v)
        {
            if (!voiceOn[v]) continue;
            activeVoices++;

            // LFO for chorus/modulation effect
            float lfo = std::sin(voices[v].phase * juce::MathConstants<float>::twoPi);
            voices[v].phase += voiceSpeed[v] / static_cast<float>(currentSampleRate);
            if (voices[v].phase >= 1.0f) voices[v].phase -= 1.0f;

            // Calculate modulated delay time in samples
            float baseDelaySamples = voiceDelayTime[v] * static_cast<float>(currentSampleRate) / 1000.0f;
            float modAmount = baseDelaySamples * voiceDepth[v] * 0.5f;
            float delayTimeSamples = baseDelaySamples + lfo * modAmount;

            // Clamp delay time to valid range
            float maxDelay = static_cast<float>(currentSampleRate) * 0.1f - 1.0f;
            delayTimeSamples = juce::jlimit(1.0f, maxDelay, delayTimeSamples);

            // Push input to delay line
            voices[v].delayLine.pushSample(0, inputL);
            voices[v].delayLine.pushSample(1, inputR);

            // Pop delayed samples
            float delayedL = voices[v].delayLine.popSample(0, delayTimeSamples);
            float delayedR = voices[v].delayLine.popSample(1, delayTimeSamples);

            // Apply distortion if either tube or bit is enabled
            if (voiceTube[v] || voiceBit[v])
            {
                if (voiceTube[v])
                {
                    delayedL = processTubeDistortion(delayedL, voiceDistortion[v]);
                    delayedR = processTubeDistortion(delayedR, voiceDistortion[v]);
                }
                if (voiceBit[v])
                {
                    delayedL = processBitCrusher(delayedL, voiceDistortion[v]);
                    delayedR = processBitCrusher(delayedR, voiceDistortion[v]);
                }
            }

            // Apply stereo width panning
            // Voice I (v=0): Center (no panning)
            // Voice II (v=1): Pan LEFT as width increases
            // Voice III (v=2): Pan RIGHT as width increases
            float voiceL = delayedL;
            float voiceR = delayedR;

            if (v == 1) // Voice II - pan to LEFT
            {
                // At width=0: center (1.0, 1.0)
                // At width=1: full left (1.0, 0.0)
                float leftGain = 1.0f;
                float rightGain = 1.0f - width;
                voiceL = delayedL * leftGain + delayedR * width * 0.5f; // Add some of R to L
                voiceR = delayedR * rightGain;
            }
            else if (v == 2) // Voice III - pan to RIGHT
            {
                // At width=0: center (1.0, 1.0)
                // At width=1: full right (0.0, 1.0)
                float leftGain = 1.0f - width;
                float rightGain = 1.0f;
                voiceL = delayedL * leftGain;
                voiceR = delayedR * rightGain + delayedL * width * 0.5f; // Add some of L to R
            }

            wetL += voiceL;
            wetR += voiceR;
        }

        // Normalize by number of active voices to prevent clipping
        if (activeVoices > 0)
        {
            float normFactor = 1.0f / std::sqrt(static_cast<float>(activeVoices));
            wetL *= normFactor;
            wetR *= normFactor;
        }

        // Calculate final output with dry/wet mix
        float outL, outR;

        if (activeVoices == 0)
        {
            // No voices active - pass dry signal through (with input gain already applied)
            outL = inputL;
            outR = inputR;
        }
        else
        {
            // Mix between dry (with input gain) and wet signals
            outL = inputL * (1.0f - mix) + wetL * mix;
            outR = inputR * (1.0f - mix) + wetR * mix;
        }

        // Apply output gain
        outL *= outputGain;
        outR *= outputGain;

        buffer.setSample(0, sample, outL);
        if (buffer.getNumChannels() > 1)
            buffer.setSample(1, sample, outR);
    }
}

bool ThreeVoicesAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ThreeVoicesAudioProcessor::createEditor()
{
    return new ThreeVoicesAudioProcessorEditor(*this);
}

void ThreeVoicesAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ThreeVoicesAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThreeVoicesAudioProcessor();
}
