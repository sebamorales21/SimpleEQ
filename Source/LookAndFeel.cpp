/*
  ==============================================================================

    LookAndFeel.cpp
    Created: 30 Dec 2025 11:20:02pm
    Author:  usuario

  ==============================================================================
*/

#include "LookAndFeel.h"

KnobLookAndFeel::KnobLookAndFeel()
{
    knobStrip = juce::ImageCache::getFromMemory(BinaryData::Main_Knob_png, BinaryData::Main_Knob_pngSize);

    jassert(!knobStrip.isNull());

    frameSize = knobStrip.getWidth();              // 64 px
    numFrames = knobStrip.getHeight() / frameSize; // 1984/64 = 31 frames
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y,
    int width, int height,
    float sliderPosProportional,
    float /*rotaryStartAngle*/,
    float /*rotaryEndAngle*/,
    juce::Slider& /*slider*/)
{
    if (knobStrip.isNull())
        return;

    const int frameIndex = juce::jlimit(0, numFrames - 1, (int)std::round(sliderPosProportional * (numFrames - 1)));

    const int srcX = 0;
    const int srcY = frameIndex * frameSize;

    const int size = juce::jmin(width, height);
    const int destX = x + (width - size) / 2;
    const int destY = y + (height - size) / 2;

    g.drawImage(knobStrip, destX, destY, size, size, srcX, srcY, frameSize, frameSize);
}