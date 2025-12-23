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

        // Speed (LFO rate in Hz) - 0 means no modulation
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "Speed", 1), "Voice " + juce::String(i) + " Speed",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f, 0.5f), 0.0f));

        // Delay Time (ms) - fixed delay offset
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(prefix + "DelayTime", 1), "Voice " + juce::String(i) + " Delay Time",
            juce::NormalisableRange<float>(0.0f, 150.0f, 0.1f), 0.0f));

        // Depth (modulation depth in ms) - FIXED amount, not relative to delay
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

        // Bit (now "Dirt" - heavier distortion) On/Off
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
    return 0.2; // 200ms to account for max delay + modulation
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
        voices[i].prepare(spec, i);
        voices[i].reset(sampleRate);
    }

    // Initialize parameter smoothing (50ms smoothing for smooth transitions)
    const float smoothingTime = 0.05f;
    smoothedInputGain.reset(sampleRate, smoothingTime);
    smoothedOutputGain.reset(sampleRate, smoothingTime);
    smoothedMix.reset(sampleRate, smoothingTime);
    smoothedWidth.reset(sampleRate, smoothingTime);

    // Initialize active gain compensation (2dB boost when voices are active)
    smoothedActiveGain.reset(sampleRate, 0.1f); // 100ms for smooth gain transitions
    smoothedActiveGain.setCurrentAndTargetValue(1.0f);

    // Reset previous delay values
    for (int i = 0; i < 3; ++i)
        prevDelayTimeSamples[i] = 0.0f;

    // Initialize per-voice parameter smoothers
    for (int i = 0; i < 3; ++i)
    {
        voiceSmoothers[i].speed.reset(sampleRate, 0.1f);  // 100ms for speed (slow transitions)
        voiceSmoothers[i].delayTime.reset(sampleRate, 0.05f); // 50ms for delay
        voiceSmoothers[i].depth.reset(sampleRate, 0.1f);  // 100ms for depth
        voiceSmoothers[i].distortion.reset(sampleRate, smoothingTime);
    }

    // Set initial values
    smoothedInputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load()));
    smoothedOutputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    smoothedMix.setCurrentAndTargetValue(apvts.getRawParameterValue("mix")->load() * 0.01f);
    smoothedWidth.setCurrentAndTargetValue(apvts.getRawParameterValue("width")->load() * 0.01f);

    // Set initial voice parameter values
    for (int i = 0; i < 3; ++i)
    {
        juce::String prefix = "voice" + juce::String(i + 1);
        float speed = apvts.getRawParameterValue(prefix + "Speed")->load();
        float delayMs = apvts.getRawParameterValue(prefix + "DelayTime")->load();
        float depth = apvts.getRawParameterValue(prefix + "Depth")->load();
        float distortion = apvts.getRawParameterValue(prefix + "Distortion")->load();

        voiceSmoothers[i].speed.setCurrentAndTargetValue(speed);
        voiceSmoothers[i].delayTime.setCurrentAndTargetValue(delayMs);
        voiceSmoothers[i].depth.setCurrentAndTargetValue(depth);
        voiceSmoothers[i].distortion.setCurrentAndTargetValue(distortion);
    }
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
    // Soft tube saturation
    float normalizedDrive = drive * 0.01f;
    if (normalizedDrive < 0.001f) return sample;

    // Apply gain before saturation
    float gainAmount = 1.0f + normalizedDrive * 5.0f;
    float x = sample * gainAmount;

    // Tanh saturation for warm tube sound
    float output = std::tanh(x);

    // Auto-gain compensation to maintain consistent level
    float compensation = 1.0f / (1.0f + normalizedDrive * 0.5f);

    return output * compensation;
}

float ThreeVoicesAudioProcessor::processBitCrusher(float sample, float drive)
{
    // RENAMED: "Dirt" - Heavy overdrive/fuzz distortion (warmer, not airy)
    float normalizedDrive = drive * 0.01f;
    if (normalizedDrive < 0.001f) return sample;

    // Asymmetric soft clipping for warm, dirty overdrive character
    float gainAmount = 1.0f + normalizedDrive * 8.0f; // More aggressive than tube
    float x = sample * gainAmount;

    // Asymmetric clipping (different positive/negative response = warmer, more analog)
    float output;
    if (x >= 0.0f)
    {
        // Positive half: softer clipping
        output = std::tanh(x * 1.2f);
    }
    else
    {
        // Negative half: slightly harder clipping for asymmetry
        output = std::tanh(x * 1.5f) * 0.9f;
    }

    // Add subtle harmonics for richness (soft rectification blend)
    float harmonics = std::abs(output) * 0.1f * normalizedDrive;
    output = output * (1.0f - normalizedDrive * 0.1f) + harmonics;

    // Auto-gain compensation
    float compensation = 1.0f / (1.0f + normalizedDrive * 0.6f);

    return output * compensation;
}

void ThreeVoicesAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Update smoothed parameter targets
    smoothedInputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load()));
    smoothedOutputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    smoothedMix.setTargetValue(apvts.getRawParameterValue("mix")->load() * 0.01f);
    smoothedWidth.setTargetValue(apvts.getRawParameterValue("width")->load() * 0.01f);

    // Read voice on/off states
    bool voiceOn[3];
    bool voiceTube[3], voiceBit[3];

    voiceOn[0] = apvts.getRawParameterValue("voice1On")->load() > 0.5f;
    voiceOn[1] = apvts.getRawParameterValue("voice2On")->load() > 0.5f;
    voiceOn[2] = apvts.getRawParameterValue("voice3On")->load() > 0.5f;

    voiceTube[0] = apvts.getRawParameterValue("voice1Tube")->load() > 0.5f;
    voiceTube[1] = apvts.getRawParameterValue("voice2Tube")->load() > 0.5f;
    voiceTube[2] = apvts.getRawParameterValue("voice3Tube")->load() > 0.5f;

    voiceBit[0] = apvts.getRawParameterValue("voice1Bit")->load() > 0.5f;
    voiceBit[1] = apvts.getRawParameterValue("voice2Bit")->load() > 0.5f;
    voiceBit[2] = apvts.getRawParameterValue("voice3Bit")->load() > 0.5f;

    // Update voice parameter smoothers
    for (int i = 0; i < 3; ++i)
    {
        const char* speedParam = (i == 0) ? "voice1Speed" : (i == 1) ? "voice2Speed" : "voice3Speed";
        const char* delayParam = (i == 0) ? "voice1DelayTime" : (i == 1) ? "voice2DelayTime" : "voice3DelayTime";
        const char* depthParam = (i == 0) ? "voice1Depth" : (i == 1) ? "voice2Depth" : "voice3Depth";
        const char* distParam = (i == 0) ? "voice1Distortion" : (i == 1) ? "voice2Distortion" : "voice3Distortion";

        voiceSmoothers[i].speed.setTargetValue(apvts.getRawParameterValue(speedParam)->load());
        voiceSmoothers[i].delayTime.setTargetValue(apvts.getRawParameterValue(delayParam)->load());
        voiceSmoothers[i].depth.setTargetValue(apvts.getRawParameterValue(depthParam)->load());
        voiceSmoothers[i].distortion.setTargetValue(apvts.getRawParameterValue(distParam)->load());
    }

    // Count active voices
    int activeVoiceCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (voiceOn[i]) activeVoiceCount++;
    }

    // Set target for active gain compensation
    // When voices are active, boost by ~2dB (1.26x) to compensate for processing
    float targetActiveGain = (activeVoiceCount > 0) ? 1.26f : 1.0f;
    smoothedActiveGain.setTargetValue(targetActiveGain);

    // Get buffer pointers
    const float* inputL = buffer.getReadPointer(0);
    const float* inputR = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : inputL;
    float* outputL = buffer.getWritePointer(0);
    float* outputR = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : outputL;

    const float sampleRateFloat = static_cast<float>(currentSampleRate);

    // Minimum delay in samples to avoid discontinuities (about 0.5ms)
    const float minDelaySamples = sampleRateFloat * 0.0005f;

    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get smoothed global parameters
        float inputGain = smoothedInputGain.getNextValue();
        float outputGain = smoothedOutputGain.getNextValue();
        float mix = smoothedMix.getNextValue();
        float width = smoothedWidth.getNextValue();
        float activeGain = smoothedActiveGain.getNextValue();

        // Get input samples with input gain
        float inL = inputL[sample] * inputGain;
        float inR = inputR[sample] * inputGain;

        // Create mono input for consistent stereo processing
        float inMono = (inL + inR) * 0.5f;

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int v = 0; v < 3; ++v)
        {
            if (!voiceOn[v]) continue;

            // Get smoothed voice parameters
            float speed = voiceSmoothers[v].speed.getNextValue();
            float delayMs = voiceSmoothers[v].delayTime.getNextValue();
            float depthPercent = voiceSmoothers[v].depth.getNextValue();
            float distortion = voiceSmoothers[v].distortion.getNextValue();

            // Convert delay time from ms to samples (LINEAR mapping: 0-150ms)
            float baseDelaySamples = delayMs * sampleRateFloat / 1000.0f;

            // Depth is a FIXED modulation amount (0-10ms), NOT relative to delay
            float maxModMs = 10.0f;
            float modAmountMs = (depthPercent * 0.01f) * maxModMs;
            float modAmountSamples = modAmountMs * sampleRateFloat / 1000.0f;

            // Calculate UNIPOLAR LFO (0 to 1)
            float lfo = 0.0f;
            if (speed > 0.001f)
            {
                float sinValue = std::sin(voices[v].phase * juce::MathConstants<float>::twoPi);
                lfo = (sinValue + 1.0f) * 0.5f;

                voices[v].phase += speed / sampleRateFloat;
                if (voices[v].phase >= 1.0f) voices[v].phase -= 1.0f;
            }

            // Final delay = base delay + LFO modulation
            float delayTimeSamples = baseDelaySamples + lfo * modAmountSamples;

            // Clamp to valid range
            float maxDelay = sampleRateFloat * 0.16f;
            delayTimeSamples = juce::jlimit(0.0f, maxDelay, delayTimeSamples);

            // Always push to delay line
            voices[v].delayLine.pushSample(0, inMono);

            // FIXED: Crossfade between dry and delayed to eliminate clicks at zero delay
            // Instead of switching abruptly, we blend based on delay amount
            float delayedSample;

            if (delayTimeSamples < minDelaySamples)
            {
                // Very low delay: use dry signal (no audible delay)
                delayedSample = inMono;
                // But still read from delay line to keep it flowing
                voices[v].delayLine.popSample(0, minDelaySamples);
            }
            else if (delayTimeSamples < minDelaySamples * 2.0f)
            {
                // Crossfade zone: blend between dry and delayed
                float crossfade = (delayTimeSamples - minDelaySamples) / minDelaySamples;
                float delayed = voices[v].delayLine.popSample(0, delayTimeSamples);
                delayedSample = inMono * (1.0f - crossfade) + delayed * crossfade;
            }
            else
            {
                // Normal delay operation
                delayedSample = voices[v].delayLine.popSample(0, delayTimeSamples);
            }

            // Apply distortion if enabled
            if (voiceTube[v] || voiceBit[v])
            {
                if (voiceTube[v])
                {
                    delayedSample = processTubeDistortion(delayedSample, distortion);
                }
                if (voiceBit[v])
                {
                    delayedSample = processBitCrusher(delayedSample, distortion);
                }
            }

            // Start with equal L/R (center panned)
            float voiceL = delayedSample;
            float voiceR = delayedSample;

            // Apply panning ONLY if 2+ voices AND width > 0
            if (activeVoiceCount >= 2 && width > 0.001f)
            {
                float panPos = 0.0f;

                if (activeVoiceCount == 2)
                {
                    int activeIndex = 0;
                    for (int check = 0; check <= v; ++check)
                    {
                        if (voiceOn[check]) activeIndex++;
                    }
                    panPos = (activeIndex == 1) ? -width : width;
                }
                else // 3 voices
                {
                    if (v == 1) panPos = -width;
                    else if (v == 2) panPos = width;
                }

                applyConstantPowerPan(voiceL, voiceR, panPos);
            }

            wetL += voiceL;
            wetR += voiceR;
        }

        // Normalize by active voices (less aggressive to maintain volume)
        if (activeVoiceCount > 0)
        {
            // Use 1/sqrt for 2-3 voices, but boost single voice
            float norm = 1.0f / std::sqrt(static_cast<float>(activeVoiceCount));
            // Additional boost to compensate for mix processing
            norm *= 1.1f;
            wetL *= norm;
            wetR *= norm;
        }

        // Mix dry/wet
        float outL, outR;
        if (activeVoiceCount == 0)
        {
            outL = inL;
            outR = inR;
        }
        else
        {
            outL = inL * (1.0f - mix) + wetL * mix;
            outR = inR * (1.0f - mix) + wetR * mix;
        }

        // Apply active gain compensation (makes plugin louder when voices are on)
        outL *= activeGain;
        outR *= activeGain;

        // Apply output gain
        outL *= outputGain;
        outR *= outputGain;

        // Apply soft limiter to prevent clipping
        outputL[sample] = softLimit(outL);
        if (totalNumOutputChannels > 1)
            outputR[sample] = softLimit(outR);
    }
}

// FIXED: Correct constant-power panning
void ThreeVoicesAudioProcessor::applyConstantPowerPan(float& left, float& right, float panPosition)
{
    // panPosition: -1.0 (full left) to 1.0 (full right), 0.0 = center
    // Map pan from [-1, 1] to [0, pi/2]
    float angle = (panPosition + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;

    // Constant-power coefficients
    float leftGain = std::cos(angle);
    float rightGain = std::sin(angle);

    // Apply to BOTH channels (mono-to-stereo panning of the voice)
    float mono = (left + right) * 0.5f;
    left = mono * leftGain;
    right = mono * rightGain;
}

// Soft limiter to prevent clipping while maintaining musicality
float ThreeVoicesAudioProcessor::softLimit(float sample)
{
    // Soft knee limiter using tanh
    // Starts limiting gently above 0.8, hard limits at ~1.0
    const float threshold = 0.8f;

    if (std::abs(sample) <= threshold)
    {
        return sample;
    }

    // Above threshold: apply soft saturation
    float sign = (sample > 0.0f) ? 1.0f : -1.0f;
    float excess = std::abs(sample) - threshold;
    float limited = threshold + (1.0f - threshold) * std::tanh(excess / (1.0f - threshold));

    return sign * limited;
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThreeVoicesAudioProcessor();
}
