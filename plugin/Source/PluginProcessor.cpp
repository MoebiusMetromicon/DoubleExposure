/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define CUSTOM_GUI 

#define GAIN_DEFAULT 1.0f
#define BLEND_DEFAULT 0.5f
#define DELAY_TIME_DEFAULT 0.3f
#define FEEDBACK_DEFAULT 0.1f
#define CHARACTER_DEFAULT 0.5f
#define MODULATION_DEFAULT 0.0f
#define DIFFUSION_DEFAULT 0.0f
#define FADE_TIME_DEFAULT 0.0f
#define SLOW_ADD_REMOVE_TRSHLD_SEC 0.8f

//==============================================================================
DoubleExposureAudioProcessor::DoubleExposureAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    previousGain(GAIN_DEFAULT),
    parameters(
        *this,
        0,
        juce::Identifier("DoubleExposure"),
        {
            std::make_unique<juce::AudioParameterFloat>("gain", "Gain", juce::NormalisableRange<float>(0.0f, 1.0f), GAIN_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("blend", "Blend", juce::NormalisableRange<float>(0.0f, 1.0f), BLEND_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", juce::NormalisableRange<float>(0.0f, 1.0f), DELAY_TIME_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", juce::NormalisableRange<float>(0.0f, 1.0f), FEEDBACK_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("modulation", "Modulation", juce::NormalisableRange<float>(0.0f, 1.0f), MODULATION_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("character", "Character", juce::NormalisableRange<float>(0.0f, 1.0f), CHARACTER_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("space", "Diffusion", juce::NormalisableRange<float>(0.0f, 1.0f), DIFFUSION_DEFAULT),
            std::make_unique<juce::AudioParameterFloat>("fadeTime", "Fade Time", juce::NormalisableRange<float>(0.0f, 1.0f), FADE_TIME_DEFAULT),

            std::make_unique<juce::AudioParameterBool>("repitchLoop", "Repitch Loop", false),
            std::make_unique<juce::AudioParameterBool>("removeOldestLoopFirst", "Remove Oldest Loop First", false),
            std::make_unique<juce::AudioParameterBool>("loopFluctuation", "Degrade Loops", false),
            std::make_unique<juce::AudioParameterBool>("pingPong", "Change Loop Time", false)
        }
    ),
    core(),
    buffers()
#endif
{
    addParameter(bypass = new juce::AudioParameterBool("bypass", "Bypass", false));
    addParameter(addLoop = new juce::AudioParameterBool("addLoop", "Add Loop", false));
    addParameter(removeLoop = new juce::AudioParameterBool("removeLoop", "Remove Loop", false));

    gain = parameters.getRawParameterValue("gain");
    blend = parameters.getRawParameterValue("blend");
    delayTime = parameters.getRawParameterValue("delayTime");
    feedback = parameters.getRawParameterValue("feedback");
    modulation = parameters.getRawParameterValue("modulation");
    character = parameters.getRawParameterValue("character");
    diffusion = parameters.getRawParameterValue("space");
    fadeTime = parameters.getRawParameterValue("fadeTime");

    repitchLoop = parameters.getRawParameterValue("repitchLoop");
    removeOldestLoopFirst = parameters.getRawParameterValue("removeOldestLoopFirst");
    loopFluctuation = parameters.getRawParameterValue("loopFluctuation");
    pingPong = parameters.getRawParameterValue("pingPong");

    buffers.tape_core_buffers = (float***)malloc(NUM_TAPE_CORES * sizeof(float**));
    for (int i = 0; i < NUM_TAPE_CORES; ++i) {
        buffers.tape_core_buffers[i] = (float**)malloc(2 * sizeof(float*));
        for (int j = 0; j < 2; ++j) {
            buffers.tape_core_buffers[i][j] = (float*)malloc(MAX_DELAY_SIZE * sizeof(float));
        }
    }
    buffers.fx_engine_buffer = (uint16_t*)malloc(FX_ENGINE_BUFFER_SIZE * sizeof(uint16_t));
}

DoubleExposureAudioProcessor::~DoubleExposureAudioProcessor()
{
    if (buffers.tape_core_buffers) {
        for (int i = 0; i < NUM_TAPE_CORES; ++i) {
            for (int j = 0; j < 2; ++j) {
                if (buffers.tape_core_buffers[i][j])
                    free(buffers.tape_core_buffers[i][j]);
            }
            free(buffers.tape_core_buffers[i]);
        }
        free(buffers.tape_core_buffers);
    }
    if (buffers.fx_engine_buffer) {
        free(buffers.fx_engine_buffer);
    }
}

//==============================================================================
const juce::String DoubleExposureAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DoubleExposureAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DoubleExposureAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DoubleExposureAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DoubleExposureAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DoubleExposureAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DoubleExposureAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DoubleExposureAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DoubleExposureAudioProcessor::getProgramName (int index)
{
    return {};
}

void DoubleExposureAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DoubleExposureAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    buffers.tape_core_buffers = (float***)malloc(NUM_TAPE_CORES * sizeof(float**));
    for (int i = 0; i < NUM_TAPE_CORES; ++i) {
        buffers.tape_core_buffers[i] = (float**)malloc(2 * sizeof(float*));
        for (int j = 0; j < 2; ++j) {
            buffers.tape_core_buffers[i][j] = (float*)malloc(MAX_DELAY_SIZE * sizeof(float));
        }
    }
    buffers.fx_engine_buffer = (uint16_t*)malloc(FX_ENGINE_BUFFER_SIZE * sizeof(uint16_t));

    previousGain = *gain;
    core.prepare(samplesPerBlock, getNumInputChannels(), sampleRate, buffers);
    core_out.setSize(2, samplesPerBlock);
}

void DoubleExposureAudioProcessor::releaseResources()
{
    if (buffers.tape_core_buffers) {
        for (int i = 0; i < NUM_TAPE_CORES; ++i) {
            for (int j = 0; j < 2; ++j) {
                if (buffers.tape_core_buffers[i][j])
                    free(buffers.tape_core_buffers[i][j]);
            }
            free(buffers.tape_core_buffers[i]);
        }
        free(buffers.tape_core_buffers);
        buffers.tape_core_buffers = 0;
    }
    if (buffers.fx_engine_buffer) {
        free(buffers.fx_engine_buffer);
        buffers.fx_engine_buffer = 0;
    }
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DoubleExposureAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DoubleExposureAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float currentGain = (*gain);
    if (juce::approximatelyEqual(currentGain, previousGain))
    {
        buffer.applyGain(currentGain);
    }
    else
    {
        buffer.applyGainRamp(0, buffer.getNumSamples(), previousGain, currentGain);
        previousGain = currentGain;
    }

    core.setParameters(
        *blend,
        *delayTime, 
        *feedback, 
        *modulation, 
        *character, 
        *diffusion,
        *fadeTime,
        *repitchLoop,
        *loopFluctuation,
        *pingPong,
        *removeOldestLoopFirst
    );

    core.add_loop_ctrl = addLoop->get();
    core.remove_loop_ctrl = removeLoop->get();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* const* buffer_data = buffer.getArrayOfWritePointers();
    auto* const* core_out_data = core_out.getArrayOfWritePointers();

    core.process(buffer_data[0], buffer_data[1], core_out_data[0], core_out_data[1], buffer.getNumSamples(), 2);

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (!(bypass->get()))
                buffer_data[channel][sample] = buffer_data[channel][sample] * (1.0 - *blend) + core_out_data[channel][sample] * *blend;
        }
    }

}

//==============================================================================
bool DoubleExposureAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DoubleExposureAudioProcessor::createEditor()
{
#ifdef CUSTOM_GUI
    return new DoubleExposureAudioProcessorEditor (*this, parameters);
#else
    return new juce::GenericAudioProcessorEditor(*this);
#endif
}

//==============================================================================
void DoubleExposureAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DoubleExposureAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName("DoubleExposure"))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoubleExposureAudioProcessor();
}
