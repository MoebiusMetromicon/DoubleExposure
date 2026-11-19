/*
  ==============================================================================

    Footswitch.h
    Created: 14 Jul 2026 3:34:03pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include<JuceHeader.h>

using namespace juce;

class Footswitch : public Button {
public:
    Footswitch() : Button("Footswitch") {
        footswitch_down = juce::ImageFileFormat::loadFrom(BinaryData::footswitch_down_png, BinaryData::footswitch_down_pngSize);
        footswitch_up = juce::ImageFileFormat::loadFrom(BinaryData::footswitch_up_png, BinaryData::footswitch_up_pngSize);
    }
    void paintButton(
        Graphics& g,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown
    ) override {
        auto area_int = getLocalBounds();
        int x = area_int.getX();
        int y = area_int.getY();
        int w = area_int.getWidth();
        int h = area_int.getHeight();

        if (isDown())
            g.drawImageWithin(footswitch_down, x, y, w, h, juce::RectanglePlacement::xLeft);
        else
            g.drawImageWithin(footswitch_up, x, y, w, h, juce::RectanglePlacement::xLeft);
    }
private:
    Image footswitch_down;
    Image footswitch_up;
};
