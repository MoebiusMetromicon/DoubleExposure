/*
  ==============================================================================

    ToggleLED.h
    Created: 16 Jul 2026 12:57:45am
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class ToggleLED : public Component {
public:
    ToggleLED(AudioParameterBool* status) :
        status(status)
    {
        led_off = juce::ImageFileFormat::loadFrom(BinaryData::led_orange_2_png, BinaryData::led_orange_2_pngSize),
        led_on = juce::ImageFileFormat::loadFrom(BinaryData::led_off_png, BinaryData::led_off_pngSize);
    };

    void paint(Graphics& g) {
        auto area_int = getLocalBounds();
        int x = area_int.getX();
        int y = area_int.getY();
        int w = area_int.getWidth();
        int h = area_int.getHeight();

        if (status->get())
            g.drawImageWithin(led_on, x, y, w, h, juce::RectanglePlacement::stretchToFit);
        else
            g.drawImageWithin(led_off, x, y, w, h, juce::RectanglePlacement::stretchToFit);
    };

private:
    AudioParameterBool* status;
    Image led_off;
    Image led_on;
};
