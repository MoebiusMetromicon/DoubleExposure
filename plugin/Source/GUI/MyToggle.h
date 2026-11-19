/*
  ==============================================================================

    MyToggle.h
    Created: 17 Jul 2026 3:30:17pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class MyToggle :
    public ToggleButton,
    public AudioProcessorValueTreeState::ButtonAttachment
{
public:
    MyToggle(AudioProcessorValueTreeState& stateToUse, const String& parameterID) :
        ButtonAttachment(stateToUse, parameterID, *this)
    {
        toggle_on = juce::ImageFileFormat::loadFrom(BinaryData::toggle_on_png, BinaryData::toggle_on_pngSize);
        toggle_off = juce::ImageFileFormat::loadFrom(BinaryData::toggle_off_png, BinaryData::toggle_off_pngSize);
    }

    void paint(Graphics& g) override {
        auto area_int = getLocalBounds();
        int x = area_int.getX();
        int y = area_int.getY();
        int w = area_int.getWidth();
        int h = area_int.getHeight();

        if (getToggleState())
            g.drawImageWithin(toggle_on, x, y, w, h, juce::RectanglePlacement::xLeft);
        else
            g.drawImageWithin(toggle_off, x, y, w, h, juce::RectanglePlacement::xLeft);
    }

private:
    Image toggle_off;
    Image toggle_on;
};
