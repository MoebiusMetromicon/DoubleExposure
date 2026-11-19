/*
  ==============================================================================

    MySlider.h
    Created: 17 Jul 2026 3:30:08pm
    Author:  julian

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

class MySlider :
    public Slider,
    public AudioProcessorValueTreeState::SliderAttachment
{
public:
    MySlider(AudioProcessorValueTreeState& stateToUse, const String& parameterID) :
        SliderAttachment(stateToUse, parameterID, *this)
    {
        knob = juce::ImageFileFormat::loadFrom(BinaryData::knob_png, BinaryData::knob_pngSize);
    }

    void paint(Graphics& g) override {
        auto area_int = getLocalBounds();
        int x = area_int.getX();
        int y = area_int.getY();
        int w = area_int.getWidth();
        int h = area_int.getHeight();

        const double rotation = (getValue()
            - getMinimum())
            / (getMaximum()
                - getMinimum());

        const int frames = knob.getHeight() / knob.getWidth();
        const int frameId = (int)ceil(rotation * ((double)frames - 1.0));
        const float radius = juce::jmin(w / 2.0f, h / 2.0f);
        const float centerX = x + w * 0.5f;
        const float centerY = y + h * 0.5f;
        const float rx = centerX - radius - 1.0f;
        const float ry = centerY - radius;

        g.drawImage(knob,
            (int)rx,
            (int)ry,
            2 * (int)radius,
            2 * (int)radius,
            0,
            frameId * knob.getWidth(),
            knob.getWidth(),
            knob.getWidth()
        );
    }

private:
    Image knob;
};
