/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define SHRINK_FACTOR 3
#define REL_X(pos) (float)pos / 160.0f
#define REL_Y(pos) (float)pos / 140.0f
#define KNOB_DIAM 27.0f
#define SIZE_COMP 2
#define LED_RATE_Hz 30



//==============================================================================
DoubleExposureAudioProcessorEditor::DoubleExposureAudioProcessorEditor(DoubleExposureAudioProcessor& p, AudioProcessorValueTreeState& vts) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    gain(vts, "gain"),
    blend(vts, "blend"),
    delay_time(vts, "delayTime"),
    feedback(vts, "feedback"),
    character(vts, "character"),
    modulation(vts, "modulation"),
    diffusion(vts, "space"),
    fade_time(vts, "fadeTime"),
    remove_oldest_loop_first(vts, "removeOldestLoopFirst"),
    ping_pong(vts, "pingPong"),
    loop_fluctuation(vts, "loopFluctuation"),
    repitch_loop(vts, "repitchLoop"),
    sliders({&gain, &blend, &delay_time, &feedback, &modulation, &character, &diffusion, &fade_time}),
    buttons({&remove_oldest_loop_first, &ping_pong, &loop_fluctuation, &repitch_loop}),
    switches({&bypass, &remove_loop, &add_loop}),
    bypass_led(p.bypass),
    delay_led(LED_RATE_Hz, &(p.getCoreRMS()[0])),
    looper_led_1(LED_RATE_Hz, &(p.getCoreRMS()[1])),
    looper_led_2(LED_RATE_Hz, &(p.getCoreRMS()[2])),
    looper_led_3(LED_RATE_Hz, &(p.getCoreRMS()[3]))
{
    background = juce::ImageFileFormat::loadFrom(BinaryData::background_png, BinaryData::background_pngSize);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(editor_w, editor_h);
    setResizable(false, false);

    addAndMakeVisible(bypass_led);
    addAndMakeVisible(delay_led);
    addAndMakeVisible(looper_led_1);
    addAndMakeVisible(looper_led_2);
    addAndMakeVisible(looper_led_3);

    for (auto slider : sliders) {
        addAndMakeVisible(slider);
        //(*slider).setLookAndFeel(&myLookAndFeel);
        (*slider).setSliderStyle(juce::Slider::RotaryVerticalDrag);
        (*slider).setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        (*slider).setRange(0.0, 1.0, 0.01);
    }
    for (auto button : buttons) {
        addAndMakeVisible(button);
        //(*button).setLookAndFeel(&myLookAndFeel);
    }
    for (auto footswitch : switches) {
        addAndMakeVisible(*footswitch);
        (*footswitch).addListener(this);
    }

    setAllComponentAreas();
}

DoubleExposureAudioProcessorEditor::~DoubleExposureAudioProcessorEditor()
{
}

//==============================================================================
void DoubleExposureAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)

    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);
}

void DoubleExposureAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    setAllComponentAreas();
}

void DoubleExposureAudioProcessorEditor::buttonClicked(juce::Button* button) {

}

void DoubleExposureAudioProcessorEditor::buttonStateChanged(juce::Button* button) {
    if (button == &bypass && button->isDown()) {
        audioProcessor.toggleBypass();
        bypass_led.repaint();
    }
    else if (button == &remove_loop)
        *audioProcessor.removeLoop = button->isDown();
    else if (button == &add_loop)
        *audioProcessor.addLoop = button->isDown();
}

