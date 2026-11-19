/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "TapeDelayLooperCore.h"
#include "juce_dsp/juce_dsp.h"

//==============================================================================
/**
*/
class DoubleExposureAudioProcessor  : public juce::AudioProcessor 
{
public:
    //==============================================================================
    DoubleExposureAudioProcessor();
    ~DoubleExposureAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    std::atomic<float>* gain;
    std::atomic<float>* blend;

    std::atomic<float>* delayTime;
    std::atomic<float>* feedback;
    std::atomic<float>* modulation;
    std::atomic<float>* character;
    std::atomic<float>* diffusion;
    std::atomic<float>* fadeTime;

    std::atomic<float>* removeOldestLoopFirst;
    std::atomic<float>* pingPong;
    std::atomic<float>* loopFluctuation;
    std::atomic<float>* repitchLoop;

    juce::AudioParameterBool* addLoop;
    juce::AudioParameterBool* removeLoop;

    juce::AudioParameterBool* bypass;

    void toggleBypass() {
        *bypass = !bypass->get();
    }

    const float const* getCoreRMS() const {
        return core.getCoreRMS();
    }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoubleExposureAudioProcessor)

    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;

    float previousGain;
    TapeDelayLooperCore core;
    juce::AudioBuffer<float> core_out;
    TapeDelayLooperBuffers buffers;

};
