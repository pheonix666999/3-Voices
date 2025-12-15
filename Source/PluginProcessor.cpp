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
            juce::NormalisableRange<float>(0.0f, 150.0f, 0.1f), 0.0f));

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
        voices[i].prepare(spec, i);
        voices[i].reset(sampleRate);
    }

    // Initialize parameter smoothing (20ms smoothing time for most, 10ms for delay)
    const float smoothingTime = 0.02f; // 20ms for most parameters
    smoothedInputGain.reset(sampleRate, smoothingTime);
    smoothedOutputGain.reset(sampleRate, smoothingTime);
    smoothedMix.reset(sampleRate, smoothingTime);
    smoothedWidth.reset(sampleRate, smoothingTime);

    // Initialize per-voice parameter smoothers
    for (int i = 0; i < 3; ++i)
    {
        voiceSmoothers[i].speed.reset(sampleRate, smoothingTime);
        voiceSmoothers[i].delayTime.reset(sampleRate, 0.01f); // 10ms for delay (faster response)
        voiceSmoothers[i].depth.reset(sampleRate, smoothingTime);
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
        float depth = apvts.getRawParameterValue(prefix + "Depth")->load() * 0.01f;
        float distortion = apvts.getRawParameterValue(prefix + "Distortion")->load();
        
        voiceSmoothers[i].speed.setCurrentAndTargetValue(speed);
        voiceSmoothers[i].delayTime.setCurrentAndTargetValue(delayMs * static_cast<float>(sampleRate) / 1000.0f);
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
    // Extended range: 0-125% (0-100 feels like 75%, so we add 25% more headroom)
    float normalizedDrive = juce::jmap(drive, 0.0f, 100.0f, 0.0f, 1.25f);
    if (normalizedDrive < 0.001f) return sample;

    // Apply more aggressive gain for dirtier sound at higher settings
    // At 100% (0.8 normalized), we get the old "100%" sound
    // At 125% (1.0 normalized), we get the new maximum
    float gainAmount = 1.0f + normalizedDrive * 4.0f;
    float x = sample * gainAmount;
    
    // Tanh saturation
    float output = std::tanh(x);
    
    // Auto-gain compensation: calculate compensation based on drive amount
    // We want the output level to stay roughly consistent, only slightly louder at max
    // Reference level at ~20% drive, compensate more at higher drives
    float referenceLevel = 0.2f; // Reference drive level
    float referenceGain = 1.0f + referenceLevel * 4.0f;
    float referenceOutput = std::tanh(1.0f * referenceGain); // Peak output at reference
    
    float currentGain = 1.0f + normalizedDrive * 4.0f;
    float currentOutput = std::tanh(1.0f * currentGain); // Peak output at current drive
    
    // Compensation factor: reduce gain as drive increases
    // Allow slight increase (up to ~20% louder at max vs zero)
    float compensation = referenceOutput / (currentOutput * 1.2f);
    compensation = juce::jlimit(0.3f, 1.0f, compensation); // Prevent over-compensation
    
    return output * compensation;
}

float ThreeVoicesAudioProcessor::processBitCrusher(float sample, float drive)
{
    // Bit crusher effect - reduces bit depth
    float amount = drive * 0.01f;
    if (amount < 0.001f) return sample;

    // Map drive to bit depth: 0% = 16 bits, 100% = 4 bits (not 2 to prevent signal cutting)
    // Use 4 bits minimum to keep signal active and audible
    float bits = juce::jmap(amount, 0.0f, 1.0f, 16.0f, 4.0f);
    float levels = std::pow(2.0f, bits);

    // Add slight soft saturation before quantization for warmer, more musical tone
    // This creates a smoother, more analog-like bit crushing character
    float preQuant = sample * 0.9f + std::tanh(sample * 2.0f) * 0.1f;
    
    // Quantize the signal
    float quantized = std::round(preQuant * levels) / levels;
    
    // Improved filtering: gentle low-pass to reduce aliasing and thin/airy character
    // Use a more effective low-pass response that increases with drive
    float filterAmount = amount * 0.35f; // More filtering for fuller tone
    // Apply gentle low-pass filtering (reduces high-frequency aliasing artifacts)
    float filtered = quantized * (1.0f - filterAmount) + quantized * 0.6f * filterAmount;
    
    // Additional gentle high-frequency rolloff for fuller, less thin sound
    // This helps reduce the "airy" aliasing artifacts
    if (amount > 0.3f)
    {
        float hfRolloff = (amount - 0.3f) * 0.2f; // Additional rolloff above 30%
        filtered *= (1.0f - hfRolloff * 0.15f); // Subtle high-frequency attenuation
    }
    
    // Auto-gain compensation: bit crushing can reduce perceived loudness
    // Compensate to keep level consistent, allow slight increase at max (similar to tube)
    float compensation = 1.0f + amount * 0.12f; // Gentle boost as bits decrease
    compensation = juce::jlimit(1.0f, 1.25f, compensation); // Max 25% boost for consistency
    
    return filtered * compensation;
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

    // Update smoothed parameter targets ONCE per block (no allocations, no String operations)
    smoothedInputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("inputGain")->load()));
    smoothedOutputGain.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("outputGain")->load()));
    smoothedMix.setTargetValue(apvts.getRawParameterValue("mix")->load() * 0.01f);
    smoothedWidth.setTargetValue(apvts.getRawParameterValue("width")->load() * 0.01f);

    // Read voice parameters ONCE per block (no String allocations - use direct parameter IDs)
    bool voiceOn[3];
    bool voiceTube[3], voiceBit[3];
    
    // Use direct parameter access to avoid String allocations
    voiceOn[0] = apvts.getRawParameterValue("voice1On")->load() > 0.5f;
    voiceOn[1] = apvts.getRawParameterValue("voice2On")->load() > 0.5f;
    voiceOn[2] = apvts.getRawParameterValue("voice3On")->load() > 0.5f;
    
    voiceTube[0] = apvts.getRawParameterValue("voice1Tube")->load() > 0.5f;
    voiceTube[1] = apvts.getRawParameterValue("voice2Tube")->load() > 0.5f;
    voiceTube[2] = apvts.getRawParameterValue("voice3Tube")->load() > 0.5f;
    
    voiceBit[0] = apvts.getRawParameterValue("voice1Bit")->load() > 0.5f;
    voiceBit[1] = apvts.getRawParameterValue("voice2Bit")->load() > 0.5f;
    voiceBit[2] = apvts.getRawParameterValue("voice3Bit")->load() > 0.5f;

    // Update smoothed voice parameter targets
    for (int i = 0; i < 3; ++i)
    {
        const char* speedParam = (i == 0) ? "voice1Speed" : (i == 1) ? "voice2Speed" : "voice3Speed";
        const char* delayParam = (i == 0) ? "voice1DelayTime" : (i == 1) ? "voice2DelayTime" : "voice3DelayTime";
        const char* depthParam = (i == 0) ? "voice1Depth" : (i == 1) ? "voice2Depth" : "voice3Depth";
        const char* distParam = (i == 0) ? "voice1Distortion" : (i == 1) ? "voice2Distortion" : "voice3Distortion";
        
        float speed = apvts.getRawParameterValue(speedParam)->load();
        float delayMs = apvts.getRawParameterValue(delayParam)->load();
        float depth = apvts.getRawParameterValue(depthParam)->load() * 0.01f;
        float distortion = apvts.getRawParameterValue(distParam)->load();
        
        voiceSmoothers[i].speed.setTargetValue(speed);
        // Convert delay time from ms to samples for smoothing
        voiceSmoothers[i].delayTime.setTargetValue(delayMs * static_cast<float>(currentSampleRate) / 1000.0f);
        voiceSmoothers[i].depth.setTargetValue(depth);
        voiceSmoothers[i].distortion.setTargetValue(distortion);
    }

    // Count active voices for panning logic
    int activeVoiceCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (voiceOn[i]) activeVoiceCount++;
    }

    // NO ALLOCATIONS: Use buffer directly, process in-place where possible
    // Get pointers to input channels (no copy needed)
    const float* inputL = buffer.getReadPointer(0);
    const float* inputR = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : inputL;
    float* outputL = buffer.getWritePointer(0);
    float* outputR = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : outputL;

    // Process each sample - NO ALLOCATIONS, all parameters smoothed per-sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get smoothed parameter values for this sample
        float inputGain = smoothedInputGain.getNextValue();
        float outputGain = smoothedOutputGain.getNextValue();
        float mix = smoothedMix.getNextValue();
        float width = smoothedWidth.getNextValue();

        // Width only works with 2+ voices, disable if only 1 voice
        if (activeVoiceCount < 2) width = 0.0f;

        // Get input samples with input gain applied (no buffer copy - direct access)
        float inputL_sample = inputL[sample] * inputGain;
        float inputR_sample = inputR[sample] * inputGain;

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int v = 0; v < 3; ++v)
        {
            if (!voiceOn[v]) continue;

            // Get smoothed voice parameters for this sample
            float smoothedSpeed = voiceSmoothers[v].speed.getNextValue();
            float smoothedBaseDelaySamples = voiceSmoothers[v].delayTime.getNextValue();
            float smoothedDepth = voiceSmoothers[v].depth.getNextValue();
            float smoothedDistortion = voiceSmoothers[v].distortion.getNextValue();

            // LFO for chorus/modulation effect (Speed is INDEPENDENT of delay time)
            // Speed controls LFO rate only - delay time is controlled by Delay Time knob
            float effectiveSpeed = smoothedSpeed * voices[v].speedMultiplier;
            float lfo = 0.0f;
            
            // Only calculate LFO if Speed > 0 (Speed=0 means no modulation, but delay still works)
            if (effectiveSpeed > 0.001f)
            {
                lfo = std::sin(voices[v].phase * juce::MathConstants<float>::twoPi);
                voices[v].phase += effectiveSpeed / static_cast<float>(currentSampleRate);
                if (voices[v].phase >= 1.0f) voices[v].phase -= 1.0f;
            }
            // If Speed = 0, lfo remains 0.0f, so no modulation but delay still works

            // Calculate delay time: baseDelay is the actual delay offset (0-150ms)
            // Depth modulates around the base delay ONLY if Speed > 0
            // If Speed = 0, delay time = baseDelay exactly (no modulation)
            float modAmountSamples = 0.0f;
            if (effectiveSpeed > 0.001f && smoothedDepth > 0.001f)
            {
                // Modulation amount: up to ±50% of base delay, or ±5ms minimum (whichever is larger)
                const float minModSamples = smoothedDepth * (0.005f * static_cast<float>(currentSampleRate)); // 5ms minimum scaled by depth
                modAmountSamples = juce::jmax(smoothedBaseDelaySamples * 0.5f * smoothedDepth, minModSamples);
            }
            
            // Final delay time: base delay ± modulation (modulation only if Speed > 0)
            float delayTimeSamples = smoothedBaseDelaySamples + lfo * modAmountSamples;

            // Clamp delay time to valid range (0-150ms)
            float maxDelay = static_cast<float>(currentSampleRate) * 0.15f;
            delayTimeSamples = juce::jlimit(0.0f, maxDelay, delayTimeSamples);
            
            // Update smoothed delay time for delay line (smooth delay changes)
            voices[v].smoothedDelaySamples.setTargetValue(delayTimeSamples);
            float finalDelaySamples = voices[v].smoothedDelaySamples.getNextValue();

            // Push input to delay line
            voices[v].delayLine.pushSample(0, inputL_sample);
            voices[v].delayLine.pushSample(1, inputR_sample);

            // Pop delayed samples using smoothed delay time (Lagrange interpolation handles smooth changes)
            float delayedL, delayedR;
            if (finalDelaySamples < 0.1f)
            {
                // At 0ms delay, use current input (no delay)
                delayedL = inputL_sample;
                delayedR = inputR_sample;
            }
            else
            {
                // Use delay line with smoothed delay time (Lagrange interpolation = click-free)
                delayedL = voices[v].delayLine.popSample(0, finalDelaySamples);
                delayedR = voices[v].delayLine.popSample(1, finalDelaySamples);
            }

            // Apply voice character filtering (subtle tone differences)
            delayedL = voices[v].filter.processSample(delayedL);
            delayedR = voices[v].filter.processSample(delayedR);

            // Apply distortion if either tube or bit is enabled (use smoothed distortion)
            if (voiceTube[v] || voiceBit[v])
            {
                if (voiceTube[v])
                {
                    delayedL = processTubeDistortion(delayedL, smoothedDistortion);
                    delayedR = processTubeDistortion(delayedR, smoothedDistortion);
                }
                if (voiceBit[v])
                {
                    delayedL = processBitCrusher(delayedL, smoothedDistortion);
                    delayedR = processBitCrusher(delayedR, smoothedDistortion);
                }
            }

            // Apply constant-power panning based on active voice combination
            float panPos = 0.0f; // -1.0 left, +1.0 right
            if (activeVoiceCount >= 2)
            {
                if (activeVoiceCount == 2)
                {
                    if (voiceOn[0] && voiceOn[1]) { panPos = (v == 0) ? -width : +width; }
                    else if (voiceOn[0] && voiceOn[2]) { panPos = (v == 0) ? -width : +width; }
                    else if (voiceOn[1] && voiceOn[2]) { panPos = (v == 1) ? -width : +width; }
                }
                else // 3 voices: I center, II left, III right
                {
                    if (v == 1) panPos = -width;
                    else if (v == 2) panPos = +width;
                    else panPos = 0.0f;
                }
            }
            // Constant-power pan
            applyConstantPowerPan(delayedL, delayedR, panPos);

            wetL += delayedL;
            wetR += delayedR;
        }

        // Normalize by number of active voices to prevent clipping
        if (activeVoiceCount > 0)
        {
            float normFactor = 1.0f / std::sqrt(static_cast<float>(activeVoiceCount));
            wetL *= normFactor;
            wetR *= normFactor;
        }

        // Calculate final output with dry/wet mix
        float outL, outR;

        if (activeVoiceCount == 0)
        {
            // No voices active - pass dry signal through (with input gain already applied)
            outL = inputL_sample;
            outR = inputR_sample;
        }
        else
        {
            // Mix between dry (with input gain) and wet signals
            outL = inputL_sample * (1.0f - mix) + wetL * mix;
            outR = inputR_sample * (1.0f - mix) + wetR * mix;
        }

        // Apply output gain
        outL *= outputGain;
        outR *= outputGain;

        // Write to output buffers (no setSample - direct pointer access)
        outputL[sample] = outL;
        if (totalNumOutputChannels > 1)
            outputR[sample] = outR;
    }
}

// Constant-power panning helper function
void ThreeVoicesAudioProcessor::applyConstantPowerPan(float& left, float& right, float panPosition)
{
    // panPosition: -1.0 (full left) to 1.0 (full right), 0.0 = center
    // Constant-power pan law: maintains perceived loudness across pan range
    float panRadians = panPosition * juce::MathConstants<float>::pi * 0.5f;
    float leftGain = std::cos(panRadians);
    float rightGain = std::sin(panRadians);
    
    float tempL = left;
    left = tempL * leftGain;
    right = right * rightGain;
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
