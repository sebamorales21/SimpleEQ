/*
  ==============================================================================

    LookAndFeel.h
    Created: 30 Dec 2025 11:11:25pm
    Author:  usuario

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// LookAndFeel personalizado para knobs tipo filmstrip (Knobman)
struct KnobLookAndFeel : public juce::LookAndFeel_V4
{
    KnobLookAndFeel();

    void drawRotarySlider(juce::Graphics& g,
        int x, int y,
        int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider) override;

private:
    juce::Image knobStrip; // Imagen completa (filmstrip)
    int frameSize = 0;    // Tamaño de cada frame (px)
    int numFrames = 0;    // Número total de frames
};
