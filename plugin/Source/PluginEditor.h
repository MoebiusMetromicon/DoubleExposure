/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/Footswitch.h"
#include "GUI/LEDMeter.h"
#include "GUI/ToggleLED.h"
#include "GUI/MySlider.h"
#include "GUI/MyToggle.h"

//==============================================================================

class DoubleExposureAudioProcessorEditor: 
    public juce::AudioProcessorEditor,
    private juce::Button::Listener
{
public:
    DoubleExposureAudioProcessorEditor (DoubleExposureAudioProcessor&, AudioProcessorValueTreeState& vts);
    ~DoubleExposureAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DoubleExposureAudioProcessor& audioProcessor;

    int editor_w = 1890 / 3;
    int editor_h = 1654 / 3;

    MySlider gain;
    MySlider blend;

    MySlider delay_time;
    MySlider feedback;
    MySlider modulation;
    MySlider character;
    MySlider diffusion;
    MySlider fade_time;

    std::vector<MySlider*> sliders;

    MyToggle remove_oldest_loop_first;
    MyToggle ping_pong;
    MyToggle loop_fluctuation;
    MyToggle repitch_loop; 

    std::vector<MyToggle*> buttons;

    Footswitch add_loop;
    Footswitch remove_loop;
    Footswitch bypass;

    std::vector<Footswitch*> switches;

    ToggleLED bypass_led;
    LEDMeter delay_led;
    LEDMeter looper_led_1;
    LEDMeter looper_led_2;
    LEDMeter looper_led_3;

    juce::Image background;

    void buttonClicked(juce::Button* button) override;
    void buttonStateChanged(juce::Button* button) override;

    enum ComponentType {
        DIAL,
        TOGGLE,
        SWITCH,
        LED
    };

    float dial_x[4] = {
        14.5f,
        49.5f,
        83.5f,
        118.5f
    };

    float toggle_x[4] = {
        22.4f,
        57.3f,
        91.3f,
        126.4f
    };

    float switch_x[3] = {
        20.2f,
        72.5f,
        124.2
    };

    float led_x[5] = {
        19.0f,
        85.3f,
        93.3f,
        101.3f,
        109.3f
    };

    juce::Rectangle<int> getElementBounds(ComponentType component_type, int id) {
        float background_w = 160.f;
        float background_h = 140.f;
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;

        switch (component_type) {
        case DIAL: {
            x = dial_x[id % 4];
            if (id <= 3)
                y = 15.0f;
            else
                y = 45.0f;
            w = h = 27.0f;
        } break;
        case TOGGLE:
            x = toggle_x[id % 4];
            y = 78.3f;
            w = 11.3f;
            h = 15.5f;
            break;
        case SWITCH:
            x = switch_x[id % 3];
            y = 105.4f;
            w = 25.7f;
            h = 13.9f;
            break;
        case LED:
            x = led_x[id % 5];
            y = 90.0f;
            w = h = 18.0f;
            break;
        default:
            break;
        }

        return {
            (int)std::round((x / background_w) * (float)editor_w),
            (int)std::round((y / background_h) * (float)editor_h),
            (int)std::round((w / background_w) * (float)editor_w),
            (int)std::round((h / background_h) * (float)editor_h)
        };
    }

    void setAllComponentAreas() {
        gain.setBounds(getElementBounds(DIAL, 3));
        delay_time.setBounds(getElementBounds(DIAL, 6));
        feedback.setBounds(getElementBounds(DIAL, 5));
        fade_time.setBounds(getElementBounds(DIAL, 7));

        blend.setBounds(getElementBounds(DIAL, 0));
        modulation.setBounds(getElementBounds(DIAL, 2));
        character.setBounds(getElementBounds(DIAL, 1));
        diffusion.setBounds(getElementBounds(DIAL, 4));

        remove_oldest_loop_first.setBounds(getElementBounds(TOGGLE, 1));
        ping_pong.setBounds(getElementBounds(TOGGLE, 0));
        loop_fluctuation.setBounds(getElementBounds(TOGGLE, 3));
        repitch_loop.setBounds(getElementBounds(TOGGLE, 2));

        bypass.setBounds(getElementBounds(SWITCH, 0));
        remove_loop.setBounds(getElementBounds(SWITCH, 1));
        add_loop.setBounds(getElementBounds(SWITCH, 2));

        bypass_led.setBounds(getElementBounds(LED, 0));
        delay_led.setBounds(getElementBounds(LED, 1));
        looper_led_1.setBounds(getElementBounds(LED, 2));
        looper_led_2.setBounds(getElementBounds(LED, 3));
        looper_led_3.setBounds(getElementBounds(LED, 4));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoubleExposureAudioProcessorEditor)
};
