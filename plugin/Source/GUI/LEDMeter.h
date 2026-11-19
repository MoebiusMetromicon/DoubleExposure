/*
  ==============================================================================

    LEDMeter.h
    Created: 15 Jul 2026 4:29:45pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#define NUM_LEVELS 8

#include <JuceHeader.h>

using namespace juce;

class LEDMeter : public Component, public Timer {
public:
    LEDMeter(int repaint_frequency_Hz, const float const * const core_out) :
        core_out(core_out)
    {
        setRepaintRate(repaint_frequency_Hz);
    }

    void timerCallback() override {
        repaint();
    }

    void setRepaintRate(int frequency_in_Hz) {
        startTimerHz(frequency_in_Hz);
    }

    void paint(Graphics& g) override {
        auto area_int = getLocalBounds();
        int x = area_int.getX();
        int y = area_int.getY();
        int w = area_int.getWidth();
        int h = area_int.getHeight();

        float level = *core_out;

        size_t led_id = 0;
        if (level < 0.001f)
            led_id = 0;
        else if (level < 0.1f)
            led_id = 1;
        else if (level < 0.15f)
            led_id = 2;
        else if (level < 0.2f)
            led_id = 3;
        else if (level < 0.3f)
            led_id = 4;
        else if (level < 0.5f)
            led_id = 5;
        else if (level < 0.9f)
            led_id = 6;
        else
            led_id = 7;

        g.drawImageWithin(led[led_id], x, y, w, h, juce::RectanglePlacement::stretchToFit);
    }

private:
    Image led[8] = {
            juce::ImageFileFormat::loadFrom(BinaryData::led_off_png, BinaryData::led_off_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_green_1_png, BinaryData::led_green_1_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_green_2_png, BinaryData::led_green_2_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_yellow_1_png, BinaryData::led_yellow_1_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_yellow_2_png, BinaryData::led_yellow_2_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_orange_1_png, BinaryData::led_orange_1_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_orange_2_png, BinaryData::led_orange_2_pngSize),
            juce::ImageFileFormat::loadFrom(BinaryData::led_red_1_png, BinaryData::led_red_1_pngSize)
    };

    float const* core_out;
    float old_level = 0.0f;
};
