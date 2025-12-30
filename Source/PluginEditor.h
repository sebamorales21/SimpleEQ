/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// CustomRotarySlider es una subclase de Slider que ya tiene el estilo rotary y sin caja de texto.
// Asi evitamos repetir ese codigo cada vez que creamos un slider de este tipo.
// CustomRotarySlider es una clase que en su cuerpo solo tiene un constructor.
// El constructor llama al constructor de la clase base Slider con los parametros adecuados.    
// El constructor de una clase derivada puede llamar al constructor de la clase base usando la sintaxis de lista de inicializacion. Como en este caso.
struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) {
    
    }
};

//==============================================================================

class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener,
	juce::Timer
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

	void parameterValueChanged(int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

	void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

	juce::Atomic<bool> parametersChanged{ false };

	CustomRotarySlider peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider, lowCutSlopeSlider, highCutSlopeSlider;
    
	// Estos Attachement son como los cables que conectan los sliders con los parametros del APVTS.
	// Deben ser inicializados en el constructor del Editor, en la lista de inicializacion.
	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;
	Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;

	std::vector<juce::Component*> getComps();

    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
